#pragma once
#include <queue>
#include <ESP32Encoder.h>
class CRotaryDialButton {
public:
	enum Button {
		BTN_NONE = 0, BTN_CLICK, BTN_LONGPRESS,
        BTN0_CLICK, BTN0_LONGPRESS,
        BTN1_CLICK, BTN1_LONGPRESS,
        BTN2_LONGPRESS,     // BTN0 + BTN1
        BTN_LEFT, BTN_RIGHT,
        BTN_LEFT_LONG, BTN_RIGHT_LONG,
        BTN_LEFT_RIGHT_LONG,
        BTN1DIAL_LONGPRESS, // BTN1 + DIAL
	};
    struct ROTARY_DIAL_SETTINGS {
        int m_nLongPressTimerValue; // mS for long press
        int m_nDialPulseCount;      // how many pulses to equal one rotation click
        bool m_bReverseDial;        // direction swapping
        bool m_bToggleDial;         // set for toggle type dial (newer PCB) and clear for pulse type dial (older PCB ones)
        int m_nDialPulseTimer;      // how long to wait to clear multiple pulses
    };
    typedef ROTARY_DIAL_SETTINGS ROTARY_DIAL_SETTINGS;
private:
    static ESP32Encoder encoder;
    static volatile int m_nButtonTimer;
    static portMUX_TYPE buttonMux;
    static ROTARY_DIAL_SETTINGS* pSettings;
    static volatile int m_nLongPressTimer;
    static esp_timer_handle_t periodic_LONGPRESS_timer;
    static esp_timer_create_args_t periodic_LONGPRESS_timer_args;
	static gpio_num_t gpioA, gpioB, gpioC, gpioBtn0, gpioBtn1, gpioAltLeft, gpioAltRight;
    static std::queue<Button> btnBuf;
    static const int m_nMaxButtons = 10;
    static volatile int m_nWaitRelease;    // this counts waits after a long press for release
#define CLICK_BUTTONS_COUNT 5
    static gpio_num_t gpioNums[CLICK_BUTTONS_COUNT]; // only the clicks, not the rotation AB ones
    // int for which one caused the interrupt
    static volatile int m_nWhichButton;
    // click array for buttons
	static Button clickBtnArray[CLICK_BUTTONS_COUNT];
	// long press array for buttons
    static Button longpressBtnArray[CLICK_BUTTONS_COUNT];
    // Private constructor so that no objects can be created.
    CRotaryDialButton() {
    }
    // the timer callback for handling long presses
    static void periodic_Button_timer_callback(void* arg)
    {
        // make sure this is a valid button
		if (m_nWhichButton<0 || m_nWhichButton>CLICK_BUTTONS_COUNT)
            return;
        // if our timer is negative, just leave
		if (!m_nWaitRelease && m_nButtonTimer < 0)
            return;
        // decrement until 0
		if (m_nButtonTimer > 0)
			--m_nButtonTimer;
        // if still not 0, nothing to do
		if (!m_nWaitRelease && m_nButtonTimer > 0)
            return;
        portENTER_CRITICAL_ISR(&buttonMux);
        Button btn;
        // let's get the level
		bool level = gpio_get_level(gpioNums[m_nWhichButton]);
        // waiting for a release after a long press was seen
		if (m_nWaitRelease) {
			if (level) {
				if (--m_nWaitRelease == 0) {
					// we are done
					m_nLongPressTimer = 0;
					m_nWhichButton = -1;
				}
            }
        }
        else {
            if (m_nLongPressTimer)
                --m_nLongPressTimer;
            // if the timer counter has finished, it must be a long press
            if (m_nLongPressTimer == 0) {
                btn = longpressBtnArray[m_nWhichButton];
                // check for both btn's on PCB down
                if ((m_nWhichButton == 1 && !gpio_get_level(gpioNums[2])) || (m_nWhichButton == 2 && !gpio_get_level(gpioNums[1]))) {
                    // make sure both buttons are still down
                    delay(1000);
                    if (!gpio_get_level(gpioNums[2]) && !gpio_get_level(gpioNums[1])) {
                        btn = BTN2_LONGPRESS;
                    }
                }
                // check for both btn1 and the dial PCB down
                if ((m_nWhichButton == 0 && !gpio_get_level(gpioNums[2])) || (m_nWhichButton == 2 && !gpio_get_level(gpioNums[0]))) {
                    // make sure both buttons are still down
                    delay(1000);
                    if (!gpio_get_level(gpioNums[2]) && !gpio_get_level(gpioNums[0])) {
                        btn = BTN1DIAL_LONGPRESS;
                    }
                }
                if (gpioNums[3] != -1 && gpioNums[4] != -1) {
                    // check for both left and right T4 btn's down
                    if ((m_nWhichButton == 3 && !gpio_get_level(gpioNums[4])) || (m_nWhichButton == 4 && !gpio_get_level(gpioNums[3]))) {
                        // make sure both buttons are still down
                        delay(1000);
                        if (!gpio_get_level(gpioNums[4]) && !gpio_get_level(gpioNums[3])) {
                            btn = BTN_LEFT_RIGHT_LONG;
                        }
                    }
                    // check for both left and center T4 btn's down
                    if ((m_nWhichButton == 3 && !gpio_get_level(gpioNums[0])) || (m_nWhichButton == 0 && !gpio_get_level(gpioNums[3]))) {
                        // make sure both buttons are still down
                        delay(1000);
                        if (!gpio_get_level(gpioNums[0]) && !gpio_get_level(gpioNums[3])) {
                            btn = BTN0_CLICK;
                        }
                    }
                    // check for both right and center T4 btn's down
                    if ((m_nWhichButton == 4 && !gpio_get_level(gpioNums[0])) || (m_nWhichButton == 0 && !gpio_get_level(gpioNums[4]))) {
                        // make sure both buttons are still down
                        delay(1000);
                        if (!gpio_get_level(gpioNums[0]) && !gpio_get_level(gpioNums[4])) {
                            btn = BTN1_CLICK;
                        }
                    }
                }
                m_nWaitRelease = 20;
                if (btnBuf.size() < m_nMaxButtons) {
                    btnBuf.push(btn);
                    m_nButtonTimer = -1;
                }
                // set it so we ignore the button interrupt for one more timer time
                m_nLongPressTimer = -1;
            }
            // if the button is up and the long timer hasn't finished counting, it must be a short press
            else if (level) {
                if (m_nLongPressTimer > 0 && m_nLongPressTimer < pSettings->m_nLongPressTimerValue - 1) {
                    btn = clickBtnArray[m_nWhichButton];
                    if (btnBuf.size() < m_nMaxButtons) {
                        btnBuf.push(btn);
                        m_nLongPressTimer = 0;
                        m_nButtonTimer = -1;
                        m_nWaitRelease = 0;
                    }
                }
            }
        }
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // button interrupt
    static void clickHandler(void* arg) {
		if (m_nButtonTimer >= 0)
            return;
        portENTER_CRITICAL_ISR(&buttonMux);
        // figure out who this was
		switch (*(char*)arg) {
		case 'C':
			m_nWhichButton = 0;
			break;
		case '0':
			m_nWhichButton = 1;
			break;
        case '1':
            m_nWhichButton = 2;
            break;
        case '2':
            m_nWhichButton = 3;
            break;
        case '3':
            m_nWhichButton = 4;
            break;
        default:
			m_nWhichButton = -1;
			break;
		}
        // our interrupt went low, if timer not started, start it
		if (m_nWhichButton != -1 && m_nLongPressTimer == 0) {
			m_nLongPressTimer = pSettings->m_nLongPressTimerValue * 10;
            m_nButtonTimer = 20; // wait 20 mS
        }
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // pull the rotary buttons from the count
    static void PullRotary()
    {
        if (gpioA == -1)
            return;
        static int nPulseCount = 1;
        static unsigned long lastTime = 0;
        portENTER_CRITICAL_ISR(&buttonMux);
		int64_t rotateCount = encoder.getCount();
        encoder.clearCount();
		while (rotateCount != 0) {
            // reset the pulsecount if it has been too long since the last button
            if (millis() > lastTime + pSettings->m_nDialPulseTimer) {
                nPulseCount = pSettings->m_nDialPulseCount;
            }
            // record the time we saw a pulse
            lastTime = millis();
            // see if there is room in the buffer
            if (btnBuf.size() < m_nMaxButtons) {
                // make sure we only count the pulses the user wants
                if (--nPulseCount == 0) {
					btnBuf.push(rotateCount > 0 ? BTN_RIGHT : BTN_LEFT);
                    nPulseCount = pSettings->m_nDialPulseCount;
                }
            }
            rotateCount > 0 ? --rotateCount : ++rotateCount;
        }
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // public things
public:
    // set the alt ones to -1, they will be ignored for T1 chip
	static void begin(gpio_num_t a, gpio_num_t b, gpio_num_t c, gpio_num_t btn0, gpio_num_t btn1, gpio_num_t altLeft, gpio_num_t altRight, ROTARY_DIAL_SETTINGS* ps) {
        // first time, set things up
        pSettings = ps;
        pSettings->m_nLongPressTimerValue = 40;
        pSettings->m_nDialPulseCount = 1;
        pSettings->m_bReverseDial = false;
        pSettings->m_bToggleDial = false;
        pSettings->m_nDialPulseTimer = 500;
        gpioA = a;
        gpioB = b;
        gpioC = c;
        gpioBtn0 = btn0;
        gpioBtn1 = btn1;
        gpioAltLeft = altLeft;
        gpioAltRight = altRight;
        // don't change order: dial click, B0, B1
        gpioNums[0] = c;
        gpioNums[1] = btn0;
        gpioNums[2] = btn1;
        gpioNums[3] = altLeft;
        gpioNums[4] = altRight;
		if (gpioA != -1) {
			// create the rotary pulse handler
			ESP32Encoder::useInternalWeakPullResistors = UP;
			encoder.attachHalfQuad(gpioB, gpioA);
			// set starting count value after attaching
			encoder.clearCount();
		}
        // create a timer
        periodic_LONGPRESS_timer_args = {
                periodic_Button_timer_callback,
                /* argument specified here will be passed to timer callback function */
                (void*)0,
                ESP_TIMER_TASK,
                "one-shotLONGPRESS"
        };
        esp_timer_create(&periodic_LONGPRESS_timer_args, &periodic_LONGPRESS_timer);
        esp_timer_start_periodic(periodic_LONGPRESS_timer, 1 * 1000);
        // pinMode() doesn't work on Heltec for pin14, strange
        // load the buttons, A and B are the dial, and C is the click
        // btn0/1 are the two buttons on the TTGO, use -1 to ignore
        if (gpioC != -1) {
            gpio_set_direction(gpioC, GPIO_MODE_INPUT);
            gpio_set_pull_mode(gpioC, GPIO_PULLUP_ONLY);
            attachInterruptArg(gpioC, clickHandler, (void*)"C", FALLING);
        }
        if (gpioBtn0 != -1) {
            gpio_set_direction(gpioBtn0, GPIO_MODE_INPUT);
            gpio_set_pull_mode(gpioBtn0, GPIO_PULLUP_ONLY);
			attachInterruptArg(gpioBtn0, clickHandler, (void*)"0", FALLING);
        }
        if (gpioBtn1 != -1) {
            gpio_set_direction(gpioBtn1, GPIO_MODE_INPUT);
            gpio_set_pull_mode(gpioBtn1, GPIO_PULLUP_ONLY);
			attachInterruptArg(gpioBtn1, clickHandler, (void*)"1", FALLING);
        }
        if (gpioAltLeft != -1) {
            gpio_set_direction(gpioAltLeft, GPIO_MODE_INPUT);
            gpio_set_pull_mode(gpioAltLeft, GPIO_PULLUP_ONLY);
            attachInterruptArg(gpioAltLeft, clickHandler, (void*)"2", FALLING);
        }
        if (gpioAltRight != -1) {
            gpio_set_direction(gpioAltRight, GPIO_MODE_INPUT);
            gpio_set_pull_mode(gpioAltRight, GPIO_PULLUP_ONLY);
            attachInterruptArg(gpioAltRight, clickHandler, (void*)"3", FALLING);
        }
        encoder.setFilter(1023);
    }
    // see what the next button is, return None if queue empty
    static Button peek()
    {
        PullRotary();
        portENTER_CRITICAL_ISR(&buttonMux);
		Button retval = BTN_NONE;
		if (!btnBuf.empty())
			retval = btnBuf.front();
        portEXIT_CRITICAL_ISR(&buttonMux);
        return retval;
    }
    // get the next button and remove from the queue, return BTN_NONE if nothing there
    static Button dequeue()
    {
        PullRotary();
        portENTER_CRITICAL_ISR(&buttonMux);
        Button btn = BTN_NONE;
        if (!btnBuf.empty()) {
            btn = btnBuf.front();
            btnBuf.pop();
        }
        portEXIT_CRITICAL_ISR(&buttonMux);
        return btn;
    }
    // wait milliseconds for a button, optionally return click or none
    // waitTime -1 means forever
    static Button waitButton(bool bClick, int waitTime = -1)
    {
        PullRotary();
        Button ret = bClick ? BTN_CLICK : BTN_NONE;
        unsigned long nowTime = millis();
        while (waitTime == -1 || millis() < nowTime + waitTime) {
            if (!btnBuf.empty()) {
                ret = dequeue();
                break;
            }
            delay(10);
        }
        return ret;
    }
    // these routines are used from user code
    // clear the buffer
    static void clear()
    {
        PullRotary();
        portENTER_CRITICAL_ISR(&buttonMux);
        while (btnBuf.size())
            btnBuf.pop();
        portEXIT_CRITICAL_ISR(&buttonMux);
    }
    // return the count
    static int getCount()
    {
        PullRotary();
        portENTER_CRITICAL_ISR(&buttonMux);
        int size = btnBuf.size();
        portEXIT_CRITICAL_ISR(&buttonMux);
        return size;
    }
    // push a button
    static void pushButton(Button btn)
    {
        portENTER_CRITICAL_ISR(&buttonMux);
        if (btnBuf.size() < m_nMaxButtons)
			btnBuf.push(btn);
        portEXIT_CRITICAL_ISR(&buttonMux);
    }
};
std::queue<enum CRotaryDialButton::Button> CRotaryDialButton::btnBuf;
gpio_num_t CRotaryDialButton::gpioNums[CLICK_BUTTONS_COUNT] = { };
gpio_num_t CRotaryDialButton::gpioA, CRotaryDialButton::gpioB, CRotaryDialButton::gpioC;
gpio_num_t CRotaryDialButton::gpioBtn0, CRotaryDialButton::gpioBtn1;
gpio_num_t CRotaryDialButton::gpioAltLeft, CRotaryDialButton::gpioAltRight;
volatile int CRotaryDialButton::m_nLongPressTimer;
volatile int CRotaryDialButton::m_nWhichButton;
esp_timer_handle_t CRotaryDialButton::periodic_LONGPRESS_timer;
esp_timer_create_args_t CRotaryDialButton::periodic_LONGPRESS_timer_args;
CRotaryDialButton::ROTARY_DIAL_SETTINGS* CRotaryDialButton::pSettings = { NULL };
CRotaryDialButton::Button CRotaryDialButton::longpressBtnArray[CLICK_BUTTONS_COUNT] = { CRotaryDialButton::BTN_LONGPRESS,CRotaryDialButton::BTN0_LONGPRESS,CRotaryDialButton::BTN1_LONGPRESS,CRotaryDialButton::BTN_LEFT_LONG,CRotaryDialButton::BTN_RIGHT_LONG };
CRotaryDialButton::Button CRotaryDialButton::clickBtnArray[CLICK_BUTTONS_COUNT] = { CRotaryDialButton::BTN_CLICK,CRotaryDialButton::BTN0_CLICK,CRotaryDialButton::BTN1_CLICK,CRotaryDialButton::BTN_LEFT,CRotaryDialButton::BTN_RIGHT };
portMUX_TYPE CRotaryDialButton::buttonMux = portMUX_INITIALIZER_UNLOCKED;
volatile int CRotaryDialButton::m_nWaitRelease = 0;
volatile int CRotaryDialButton::m_nButtonTimer = -1;
ESP32Encoder CRotaryDialButton::encoder;
