#pragma once
#include <queue>
class CRotaryDialButton {
public:
	enum Button {
		BTN_NONE = 0, BTN_CLICK, BTN_LONGPRESS,
        BTN0_CLICK, BTN0_LONGPRESS,
        BTN1_CLICK, BTN1_LONGPRESS,
        BTN2_LONGPRESS,
        BTN_LEFT, BTN_RIGHT,
	};
    struct ROTARY_DIAL_SETTINGS {
        int m_nLongPressTimerValue; // mS for long press
        int m_nDialPulseCount;      // how many pulse to equal one rotation click
        int m_nDialSpeed;           // pause after each dial click
        bool m_bReverseDial;        // direction swapping
        bool m_bToggleDial;         // set for toggle type dial (newer PCB) and clear for pulse type dial (older PCB ones)
        bool m_bAcceleration;       // set to accelerate fast rotations, I.E. more clicks per click when fast
    };
    typedef ROTARY_DIAL_SETTINGS ROTARY_DIAL_SETTINGS;
private:
    static volatile int m_nButtonTimer;
    static portMUX_TYPE buttonMux;
    static ROTARY_DIAL_SETTINGS* pSettings;
    static volatile int m_nLongPressTimer;
    static esp_timer_handle_t periodic_LONGPRESS_timer, rotary_timer;
    static esp_timer_create_args_t periodic_LONGPRESS_timer_args, rotary_timer_args;
    static gpio_num_t gpioA, gpioB, gpioC, gpioBtn0, gpioBtn1;
    static std::queue<Button> btnBuf;
    static const int m_nMaxButtons = 4;
    static volatile bool m_bWaitingForRotary;
    static volatile int m_nWaitRelease;    // this counts waits after a long press for release
#define CLICK_BUTTONS_COUNT 3
    static gpio_num_t gpioNums[CLICK_BUTTONS_COUNT]; // only the clicks, not the rotation AB ones
    // int for which one caused the interrupt
    static volatile int whichButton;
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
		if (whichButton<0 || whichButton>CLICK_BUTTONS_COUNT)
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
		bool level = gpio_get_level(gpioNums[whichButton]);
        // waiting for a release after a long press was seen
		if (m_nWaitRelease) {
			if (level) {
				if (--m_nWaitRelease == 0) {
					// we are done
					m_nLongPressTimer = 0;
					whichButton = -1;
				}
            }
        }
        else {
            if (m_nLongPressTimer)
                --m_nLongPressTimer;
            // if the timer counter has finished, it must be a long press
            if (m_nLongPressTimer == 0) {
                btn = longpressBtnArray[whichButton];
                // check for both btn's on PCB down
                if ((whichButton == 1 && !gpio_get_level(gpioNums[2])) || (whichButton == 2 && !gpio_get_level(gpioNums[1]))) {
                    // make sure both buttons are still down
                    delay(1000);
                    if (!gpio_get_level(gpioNums[2]) && !gpio_get_level(gpioNums[1])) {
                        btn = BTN2_LONGPRESS;
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
                    btn = clickBtnArray[whichButton];
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
			whichButton = 0;
			break;
		case '0':
			whichButton = 1;
			break;
		case '1':
			whichButton = 2;
			break;
		default:
			whichButton = -1;
			break;
		}
        // our interrupt went low, if timer not started, start it
		if (whichButton != -1 && m_nLongPressTimer == 0) {
			m_nLongPressTimer = pSettings->m_nLongPressTimerValue * 10;
            m_nButtonTimer = 20; // wait 20 mS
        }
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // interrupt routines for the A and B rotary switch contacts
    // basically it gets interrupts from the A side and then looks at B to see which direction it was going
    // there is also a counter that will require 1 or more pulses before the rotation is queued
    // Some switches pulse closed and then back to open while others just switch state, this code
    // should work with both kinds
    // the dialspeed is how many mS to go deaf after the interrupt, this handles switch bounce as well as
    // slowing down the max rotation speed of the dial
    static void rotateHandler(void* arg) {
        //if (*(char*)arg != 'A')
        //    return;
        if (m_bWaitingForRotary)
            return;
        portENTER_CRITICAL_ISR(&buttonMux);
        // kill the long press timer for the other buttons
        m_nLongPressTimer = 0;
        whichButton = -1;
        m_nButtonTimer = -1;
        m_nWaitRelease = 0;

        static unsigned long lastTime = 0;
        static unsigned long lastButtonPush = 0;
        // ignore pulses if too soon since the last int
        if (millis() < lastTime + pSettings->m_nDialSpeed) {
            portEXIT_CRITICAL_ISR(&buttonMux);
            return;
        }
        lastTime = millis();
        // let the switch settle down, and handle it in the timer
        m_bWaitingForRotary = true;
        esp_timer_start_once(rotary_timer, 5000);
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // timer to handle final rotary dial actions
    static void rotary_timer_callback(void* arg)
    {
        // count of buttons for when the sensitivity is reduced from nButtonSensitivity
        static unsigned int countRight = 0;
        static unsigned int countLeft = 0;
        static unsigned long lastButtonPush = 0;
        portENTER_CRITICAL_ISR(&buttonMux);
        m_bWaitingForRotary = false;
        bool valA = gpio_get_level(gpioA);
        bool valB = gpio_get_level(gpioB);
        if (valA && !pSettings->m_bToggleDial) {
            portEXIT_CRITICAL_ISR(&buttonMux);
            return;
        }
        Button btnToPush = BTN_NONE;
        bool cmpBtn = pSettings->m_bReverseDial ? (valA == valB) : (valA != valB);
        Button btn = cmpBtn ? BTN_RIGHT : BTN_LEFT;
        // see if we are counting pulses, increment the correct one
        if (btn == BTN_RIGHT)
            ++countRight;
        else if (btn == BTN_LEFT)
            ++countLeft;
        // if both have a value, clear
        if (countLeft && countRight)
            countLeft = countRight = 0;
        // see if we have enough
        if (countLeft >= pSettings->m_nDialPulseCount) {
            btnToPush = BTN_LEFT;
            countLeft = 0;
        }
        else if (countRight >= pSettings->m_nDialPulseCount) {
            btnToPush = BTN_RIGHT;
            countRight = 0;
        }
        unsigned long diff = millis() - lastButtonPush;
        int count = 1;
        if (pSettings->m_bAcceleration && diff < 100) {
            count = 200 / diff;
            //Serial.println("diff: " + String(diff) + " " + String(count));
        }
        while (btnToPush != BTN_NONE && (btnToPush == BTN_RIGHT || btnToPush == BTN_LEFT) && count--) {
            if (btnBuf.size() < m_nMaxButtons) {
                btnBuf.push(btnToPush);
            }
        }
        lastButtonPush = millis();
        portEXIT_CRITICAL_ISR(&buttonMux);
    }

    // public things
public:
	static void begin(gpio_num_t a, gpio_num_t b, gpio_num_t c, gpio_num_t btn0, gpio_num_t btn1, ROTARY_DIAL_SETTINGS* ps) {
        // first time, set things up
        pSettings = ps;
        pSettings->m_nLongPressTimerValue = 40;
        pSettings->m_nDialPulseCount = 1;
        pSettings->m_nDialSpeed = 10;
        pSettings->m_bReverseDial = false;
        pSettings->m_bToggleDial = false;
        pSettings->m_bAcceleration = false;
        gpioA = a;
        gpioB = b;
        gpioC = c;
        gpioBtn0 = btn0;
        gpioBtn1 = btn1;
        // don't change order: dial click, B0, B1
        gpioNums[0] = c;
        gpioNums[1] = btn0;
        gpioNums[2] = btn1;
        // create a timer
        // set the interrupts
        periodic_LONGPRESS_timer_args = {
                periodic_Button_timer_callback,
                /* argument specified here will be passed to timer callback function */
                (void*)0,
                ESP_TIMER_TASK,
                "one-shotLONGPRESS"
        };
        esp_timer_create(&periodic_LONGPRESS_timer_args, &periodic_LONGPRESS_timer);
        esp_timer_start_periodic(periodic_LONGPRESS_timer, 1 * 1000);
        rotary_timer_args = {
                rotary_timer_callback,
                /* argument specified here will be passed to timer callback function */
                (void*)0,
                ESP_TIMER_TASK,
                "one-shotROTARY"
        };
        esp_timer_create(&rotary_timer_args, &rotary_timer);
        // pinMode() doesn't work on Heltec for pin14, strange
        // load the buttons, A and B are the dial, and C is the click
        // btn0/1 are the two buttons on the TTGO, use -1 to ignore
        gpio_set_direction(gpioA, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpioA, GPIO_PULLUP_ONLY);
        gpio_set_direction(gpioB, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpioB, GPIO_PULLUP_ONLY);
        gpio_set_direction(gpioC, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpioC, GPIO_PULLUP_ONLY);
        //gpio_isr_handler_add(gpioC, clickHandler, (void*)"C");
        //gpio_set_intr_type(gpioC, GPIO_INTR_NEGEDGE);
		attachInterruptArg(gpioC, clickHandler, (void*)"C", FALLING);
		attachInterruptArg(gpioA, rotateHandler, (void*)"A", CHANGE);
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
    }
    // see what the next button is, return None if queue empty
    static Button peek()
    {
        noInterrupts();
		Button retval = BTN_NONE;
		if (!btnBuf.empty())
			retval = btnBuf.front();
        interrupts();
    }
    // get the next button and remove from the queue, return BTN_NONE if nothing there
    static Button dequeue()
    {
        noInterrupts();
        Button btn = BTN_NONE;
        if (!btnBuf.empty()) {
            btn = btnBuf.front();
            btnBuf.pop();
        }
        interrupts();
        return btn;
    }
    // wait milliseconds for a button, optionally return click or none
    // waitTime -1 means forever
    static Button waitButton(bool bClick, int waitTime = -1)
    {
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
        noInterrupts();
        while (btnBuf.size())
            btnBuf.pop();
        interrupts();
    }
    // return the count
    static int getCount()
    {
        noInterrupts();
        int size = btnBuf.size();
        interrupts();
        return size;
    }
    // push a button
    static void pushButton(Button btn)
    {
        noInterrupts();
		if (btnBuf.size() < m_nMaxButtons)
			btnBuf.push(btn);
        interrupts();
    }
};
std::queue<enum CRotaryDialButton::Button> CRotaryDialButton::btnBuf;
gpio_num_t CRotaryDialButton::gpioNums[CLICK_BUTTONS_COUNT] = { };
gpio_num_t CRotaryDialButton::gpioA, CRotaryDialButton::gpioB, CRotaryDialButton::gpioC;
gpio_num_t CRotaryDialButton::gpioBtn0, CRotaryDialButton::gpioBtn1;
volatile int CRotaryDialButton::m_nLongPressTimer;
volatile int CRotaryDialButton::whichButton;
esp_timer_handle_t CRotaryDialButton::periodic_LONGPRESS_timer, CRotaryDialButton::rotary_timer;
esp_timer_create_args_t CRotaryDialButton::periodic_LONGPRESS_timer_args, CRotaryDialButton::rotary_timer_args;
CRotaryDialButton::ROTARY_DIAL_SETTINGS* CRotaryDialButton::pSettings = { NULL };
CRotaryDialButton::Button CRotaryDialButton::longpressBtnArray[CLICK_BUTTONS_COUNT] = { CRotaryDialButton::BTN_LONGPRESS,CRotaryDialButton::BTN0_LONGPRESS,CRotaryDialButton::BTN1_LONGPRESS };
CRotaryDialButton::Button CRotaryDialButton::clickBtnArray[CLICK_BUTTONS_COUNT] = { CRotaryDialButton::BTN_CLICK,CRotaryDialButton::BTN0_CLICK,CRotaryDialButton::BTN1_CLICK };
portMUX_TYPE CRotaryDialButton::buttonMux = portMUX_INITIALIZER_UNLOCKED;
volatile int CRotaryDialButton::m_nWaitRelease = 0;
volatile int CRotaryDialButton::m_nButtonTimer = -1;
volatile bool CRotaryDialButton::m_bWaitingForRotary = false;