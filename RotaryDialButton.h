#pragma once
#include <queue>
class CRotaryDialButton {
public:
	enum Button { BTN_NONE, BTN_LEFT, BTN_RIGHT, BTN_CLICK, BTN_LONGPRESS };
    static int m_nLongPressTimerValue;
    static int m_nDialSensitivity;
    static int m_nDialSpeed;
    static bool m_bReverseDial;
private:
    static CRotaryDialButton* instance;
    static std::queue<enum Button> btnBuf;
    static volatile int m_nLongPressTimer;
    static esp_timer_handle_t periodic_LONGPRESS_timer;
    static esp_timer_create_args_t periodic_LONGPRESS_timer_args;
    static int gpioA, gpioB, gpioC;
    // Private constructor so that no objects can be created.
    CRotaryDialButton() {
    }

public:
    static CRotaryDialButton* getInstance() {
        if (!instance)
            instance = new CRotaryDialButton();

        return instance;
    }
private:
    // the timer callback for handling long presses
    static void periodic_LONGPRESS_timer_callback(void* arg)
    {
        noInterrupts();
        enum Button btn;
        bool level = digitalRead(gpioC);
        --m_nLongPressTimer;
        // if the timer counter has finished, it must be a long press
        if (m_nLongPressTimer == 0) {
            btn = BTN_LONGPRESS;
            btnBuf.push(btn);
            // set it so we ignore the button interrupt for one more timer time
            m_nLongPressTimer = -1;
        }
        // if the button is up and the timer hasn't finished counting, it must be a short press
        else if (level) {
            if (m_nLongPressTimer > 0 && m_nLongPressTimer < m_nLongPressTimerValue - 1) {
                btn = BTN_CLICK;
                btnBuf.push(btn);
                m_nLongPressTimer = -1;
            }
            else if (m_nLongPressTimer < -1) {
                esp_timer_stop(periodic_LONGPRESS_timer);
                m_nLongPressTimer = 0;
            }
        }
        interrupts();
    }
    // button interrupt
    static void clickHandler() {
        noInterrupts();
        // went low, if timer not started, start it
        if (m_nLongPressTimer == 0) {
            m_nLongPressTimer = m_nLongPressTimerValue;
            esp_timer_stop(periodic_LONGPRESS_timer);	// just in case
            esp_timer_start_periodic(periodic_LONGPRESS_timer, 10 * 1000);
        }
        interrupts();
    }
    // interrupt routines for the A and B rotary switches
    // the pendingBtn is used to hold the right rotation until A goes up, this makes
    // the left and right rotation happen at the same apparent time when rotating the dial,
    // unlike without that where the right rotation was faster, I.E. it moved before the
    // click of the button happened.
    static void rotateHandler() {
        noInterrupts();
        // count of buttons for when the sensitivity is reduced from nButtonSensitivity
        static unsigned int countRight = 0;
        static unsigned int countLeft = 0;
        static enum Button pendingBtn = BTN_NONE;
        static unsigned long lastTime;
        static bool lastValA = true;
        bool valA = digitalRead(gpioA);
        bool valB = digitalRead(gpioB);
        enum Button btnToPush = BTN_NONE;
        // ignore until the time has expired
        unsigned long millisNow = millis();
        if (millisNow - lastTime > m_nDialSpeed) {
            // been too long, reset the counts
            countRight = countLeft = 0;
            // and the pending one
            pendingBtn = BTN_NONE;
        }
        if (lastValA != valA && millisNow > lastTime + 3) {
            if (pendingBtn != BTN_NONE) {
                btnToPush = pendingBtn;
                pendingBtn = BTN_NONE;
            }
            else if (lastValA && !valA) {
                enum Button btn = (m_bReverseDial ? !valB : valB) ? BTN_RIGHT : BTN_LEFT;
                if (btn == BTN_RIGHT) {
                    pendingBtn = btn;
                }
                else {
                    btnToPush = btn;
                }
            }
            lastValA = valA;
            // push a button?
            if (btnToPush != BTN_NONE) {
                // check sensitivity counts
                if (btnToPush == BTN_RIGHT)
                    ++countRight;
                else
                    ++countLeft;
                if (countRight >= m_nDialSensitivity || countLeft >= m_nDialSensitivity) {
                    btnBuf.push(btnToPush);
                    countRight = countLeft = 0;
                }
            }
            // remember when we were here last time
            lastTime = millisNow;
        }
        interrupts();
    }
public:
	static void begin(int a, int b, int c) {
        // first time, set things up
        m_nLongPressTimerValue = 40;
        m_nDialSensitivity = 1;
        m_nDialSpeed = 300;
        m_bReverseDial = false;
        gpioA = a;
        gpioB = b;
        gpioC = c;
        // pinMode() doesn't work on Heltec for pin14, strange
        // load the buttons, A and B are the dial, and C is the click
        gpio_set_direction((gpio_num_t)a, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)a, GPIO_PULLUP_ONLY);
        gpio_set_direction((gpio_num_t)b, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)b, GPIO_PULLUP_ONLY);
        gpio_set_direction((gpio_num_t)c, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)c, GPIO_PULLUP_ONLY);
        // create a timer
        // set the interrupts
        periodic_LONGPRESS_timer_args = {
                periodic_LONGPRESS_timer_callback,
                /* argument specified here will be passed to timer callback function */
                (void*)0,
                ESP_TIMER_TASK,
                "one-shotLONGPRESS"
        };
        esp_timer_create(&periodic_LONGPRESS_timer_args, &periodic_LONGPRESS_timer);
        attachInterrupt(gpioC, clickHandler, FALLING);
        attachInterrupt(gpioA, rotateHandler, CHANGE);
    }
    // see what the next button is, return None if queue empty
    static enum Button peek() {
        if (btnBuf.empty())
            return BTN_NONE;
        return btnBuf.front();
    }
    // get the next button and remove from the queue, return BTN_NONE if nothing there
    static enum Button dequeue() {
        enum Button btn = BTN_NONE;
        if (!btnBuf.empty()) {
            btn = btnBuf.front();
            btnBuf.pop();
        }
        return btn;
    }
    // wait milliseconds for a button, optionally return click or none
	static enum Button waitButton(bool bClick, int waitTime) {
        enum Button ret = bClick ? BTN_CLICK : BTN_NONE;
        unsigned long nowTime = millis();
        while (millis() < nowTime + waitTime) {
            if (!btnBuf.empty()) {
                ret = dequeue();
                break;
            }
            delay(10);
        }
        return ret;
    }
    // clear the buffer
    static void clear() {
        while (btnBuf.size())
            btnBuf.pop();
    }
    // return the count
    static int getCount() {
        return btnBuf.size();
    }
    // push a button
    static void pushButton(enum Button btn) {
        btnBuf.push(btn);
    }
};
//Initialize pointer to zero so that it can be initialized in first call to getInstance
CRotaryDialButton* CRotaryDialButton::instance = NULL;
std::queue<enum CRotaryDialButton::Button> CRotaryDialButton::btnBuf;
int CRotaryDialButton::m_nLongPressTimerValue;
volatile int CRotaryDialButton::m_nLongPressTimer;
int CRotaryDialButton::m_nDialSensitivity;
int CRotaryDialButton::m_nDialSpeed;
bool CRotaryDialButton::m_bReverseDial;
esp_timer_handle_t CRotaryDialButton::periodic_LONGPRESS_timer;
esp_timer_create_args_t CRotaryDialButton::periodic_LONGPRESS_timer_args;
int CRotaryDialButton::gpioA, CRotaryDialButton::gpioB, CRotaryDialButton::gpioC;
