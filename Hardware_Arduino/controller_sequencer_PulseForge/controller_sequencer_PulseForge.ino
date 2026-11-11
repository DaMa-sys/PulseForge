/*
 * ==============================================================================
 * DaMA PulseForge - Dedicated Hardware Desktop Controller
 * Target: Arduino Mega 2560 (ATmega2560)
 * Communication: High-Speed Serial @ 115200 Baud with Auto-Discovery Handshake
 *
 * Requirements:
 * - Library "U8g2" by olikraus (install via Arduino Library Manager)
 * ==============================================================================
 */

#include <Arduino.h>

//==============================================================================
// IMPOSTAZIONI HARDWARE:
// De-commenta la riga sotto SOLO quando avrai collegato gli schermi OLED
// e avrai installato la libreria 'U8g2' dall'Arduino IDE.
// Lasciala commentata per ora così lo sketch funziona subito senza dipendenze!
//==============================================================================
// #define ENABLE_OLED 1

#ifdef ENABLE_OLED
  #include <U8g2lib.h>
#endif

//==============================================================================
// 1. PIN DEFINITIONS
//==============================================================================

// 15 Analog Inputs (Direct to Arduino Mega ADC)
const uint8_t POT_PINS[3][5] = {
    { A0,  A1,  A2,  A3,  A4 },   // Track 1 (Bass):   Density, Swing, BaseOct, ArpOct, Ratio
    { A5,  A6,  A7,  A8,  A9 },   // Track 2 (Arp):    Density, Swing, BaseOct, ArpOct, Ratio
    { A10, A11, A12, A13, A14 }   // Track 3 (Chords): Density, Swing, BaseOct, ArpOct, Ratio
};

// 6 Tactile Buttons (INPUT_PULLUP)
const uint8_t BTN_GEN[3] = { 2, 4, 6 };
const uint8_t BTN_MUT[3] = { 3, 5, 7 };

// 3 Beat / Step Indicator LEDs
const uint8_t LED_PINS[3] = { 8, 9, 10 };

// 3 OLED Displays (Software I2C: Clock, Data)
#ifdef ENABLE_OLED
U8G2_SSD1306_128X64_NONAME_1_SW_I2C oled1 (U8G2_R0, /* scl=*/ 12, /* sda=*/ 11, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_SW_I2C oled2 (U8G2_R0, /* scl=*/ 15, /* sda=*/ 14, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_SW_I2C oled3 (U8G2_R0, /* scl=*/ 17, /* sda=*/ 16, /* reset=*/ U8X8_PIN_NONE);
U8G2* oleds[3] = { &oled1, &oled2, &oled3 };
#endif

// 9 Rotary Encoders (CLK, DT, SW)
struct EncoderPins {
    uint8_t pinCLK;
    uint8_t pinDT;
    uint8_t pinSW;
};

const EncoderPins ENCODERS[3][3] = {
    // Track 1: Genre, Role, Steps
    { { 22, 23, 24 }, { 25, 26, 27 }, { 28, 29, 30 } },
    // Track 2: Genre, Role, Steps
    { { 31, 32, 33 }, { 34, 35, 36 }, { 37, 38, 39 } },
    // Track 3: Genre, Role, Steps
    { { 40, 41, 42 }, { 43, 44, 45 }, { 46, 47, 48 } }
};

//==============================================================================
// 2. STATE & BUFFERING (Anti-jitter & Debounce)
//==============================================================================

// Potentiometer readings (0..1023 mapped to 0..100 or target ranges)
int lastPotRaw[3][5];
int lastSentPotVal[3][5] = {
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 }
};
unsigned long lastPotReadTime = 0;

// Button debounce
bool lastBtnGenState[3] = { HIGH, HIGH, HIGH };
bool lastBtnMutState[3] = { HIGH, HIGH, HIGH };
unsigned long lastBtnDebounceTime[3] = { 0, 0, 0 };

// Encoder states
uint8_t lastEncoderState[3][3];
bool lastEncoderBtnState[3][3];

// LED flash timers
unsigned long ledOffTime[3] = { 0, 0, 0 };

// OLED Display text buffers
char oledLine1[3][16] = { "SEQ 1: BASS",   "SEQ 2: ARP",    "SEQ 3: CHORD" };
char oledLine2[3][16] = { "Mono Bass",     "Classic Arp",   "Harmony Chords" };
char oledLine3[3][16] = { "C0 | 16 Steps", "C1 | 16 Steps", "Cm9 | 32 Steps" };
bool oledDirty[3] = { true, true, true };

// Serial input buffer
char rxBuffer[64];
uint8_t rxIndex = 0;

// Host Connection State (Inputs disabled until connected to prevent noise flooding)
bool isConnectedToHost = false;
unsigned long lastHostActivity = 0;

//==============================================================================
// 3. SETUP
//==============================================================================
void setup()
{
    Serial.begin (115200);

    // Initialize Buttons & LEDs
    for (int t = 0; t < 3; ++t)
    {
        pinMode (BTN_GEN[t], INPUT_PULLUP);
        pinMode (BTN_MUT[t], INPUT_PULLUP);

        pinMode (LED_PINS[t], OUTPUT);
        digitalWrite (LED_PINS[t], LOW);

        for (int p = 0; p < 5; ++p)
        {
            lastPotRaw[t][p] = analogRead (POT_PINS[t][p]);
        }

        for (int e = 0; e < 3; ++e)
        {
            pinMode (ENCODERS[t][e].pinCLK, INPUT_PULLUP);
            pinMode (ENCODERS[t][e].pinDT,  INPUT_PULLUP);
            pinMode (ENCODERS[t][e].pinSW,  INPUT_PULLUP);
            lastEncoderState[t][e] = digitalRead (ENCODERS[t][e].pinCLK);
            lastEncoderBtnState[t][e] = HIGH;
        }

        // Initialize OLED
#ifdef ENABLE_OLED
        oleds[t]->begin();
#endif
    }

    // Startup LED welcome flash
    for (int i = 0; i < 2; ++i)
    {
        for (int t = 0; t < 3; ++t) digitalWrite (LED_PINS[t], HIGH);
        delay (80);
        for (int t = 0; t < 3; ++t) digitalWrite (LED_PINS[t], LOW);
        delay (80);
    }
}

//==============================================================================
// 4. OLED RENDERING (Custom UI Strisce)
//==============================================================================
void updateOLED (int t)
{
#ifdef ENABLE_OLED
    if (!oledDirty[t])
        return;

    oleds[t]->firstPage();
    do {
        // Header
        oleds[t]->setFont (u8g2_font_6x10_tf);
        oleds[t]->drawStr (0, 10, oledLine1[t]);
        oleds[t]->drawHLine (0, 13, 128);

        // Subtitle / Role
        oleds[t]->setFont (u8g2_font_7x14B_tf);
        oleds[t]->drawStr (0, 32, oledLine2[t]);

        // Details / Chord / Pitch / Steps
        oleds[t]->setFont (u8g2_font_9x15B_tf);
        oleds[t]->drawStr (0, 56, oledLine3[t]);
    } while (oleds[t]->nextPage());

    oledDirty[t] = false;
#else
    (void) t;
#endif
}

//==============================================================================
// 5. INPUT SCANNERS
//==============================================================================

void scanButtons()
{
    const unsigned long now = millis();
    for (int t = 0; t < 3; ++t)
    {
        // 1. GENERATE Button
        int gVal = digitalRead (BTN_GEN[t]);
        if (gVal != lastBtnGenState[t] && (now - lastBtnDebounceTime[t] > 50))
        {
            lastBtnDebounceTime[t] = now;
            lastBtnGenState[t] = gVal;
            if (gVal == LOW) // Pressed
            {
                Serial.print ("T"); Serial.print (t); Serial.println (":GEN");
                digitalWrite (LED_PINS[t], HIGH);
                ledOffTime[t] = now + 60;
            }
        }

        // 2. MUTATE Button
        int mVal = digitalRead (BTN_MUT[t]);
        if (mVal != lastBtnMutState[t] && (now - lastBtnDebounceTime[t] > 50))
        {
            lastBtnDebounceTime[t] = now;
            lastBtnMutState[t] = mVal;
            if (mVal == LOW)
            {
                Serial.print ("T"); Serial.print (t); Serial.println (":MUT");
                digitalWrite (LED_PINS[t], HIGH);
                ledOffTime[t] = now + 60;
            }
        }
    }
}

void scanPotentiometers()
{
    const unsigned long now = millis();
    if (now - lastPotReadTime < 40) // Scan every 40ms (~25Hz)
        return;
    lastPotReadTime = now;

    static const char* potNames[5] = { "DENS", "SWING", "BOCT", "AOCT", "RATIO" };

    for (int t = 0; t < 3; ++t)
    {
        for (int p = 0; p < 5; ++p)
        {
            int raw = analogRead (POT_PINS[t][p]);

            // Deadband threshold to eliminate ADC noise jitter
            if (abs (raw - lastPotRaw[t][p]) > 14)
            {
                lastPotRaw[t][p] = raw;

                int val = 0;
                if (p == 0)      val = map (raw, 0, 1023, 0, 100);  // Density 0..100
                else if (p == 1) val = map (raw, 0, 1023, 0, 75);   // Swing 0..75
                else if (p == 2) val = map (raw, 0, 1023, 0, 5);    // Base Oct 0..5
                else if (p == 3) val = map (raw, 0, 1023, 1, 4);    // Arp Oct 1..4
                else if (p == 4) val = map (raw, 0, 1023, 0, 10);   // Tempo Ratio 0..10

                // Only send over Serial if the mapped discrete value actually changed
                if (val != lastSentPotVal[t][p])
                {
                    lastSentPotVal[t][p] = val;
                    Serial.print ("T"); Serial.print (t);
                    Serial.print (":"); Serial.print (potNames[p]);
                    Serial.print (":"); Serial.println (val);
                }
            }
        }
    }
}

void scanEncoders()
{
    static const char* encNames[3] = { "GENRE", "ROLE", "STEPS" };

    for (int t = 0; t < 3; ++t)
    {
        for (int e = 0; e < 3; ++e)
        {
            uint8_t clk = digitalRead (ENCODERS[t][e].pinCLK);
            if (clk != lastEncoderState[t][e] && clk == LOW)
            {
                // Falling edge on CLK: check DT for direction
                int dir = (digitalRead (ENCODERS[t][e].pinDT) != clk) ? 1 : -1;
                Serial.print ("T"); Serial.print (t);
                Serial.print (":ENC:"); Serial.print (encNames[e]);
                Serial.print (":"); Serial.println (dir > 0 ? "+1" : "-1");
            }
            lastEncoderState[t][e] = clk;

            // Encoder Push-Button
            int sw = digitalRead (ENCODERS[t][e].pinSW);
            if (sw != lastEncoderBtnState[t][e])
            {
                lastEncoderBtnState[t][e] = sw;
                if (sw == LOW)
                {
                    Serial.print ("T"); Serial.print (t);
                    Serial.print (":CLICK:"); Serial.println (encNames[e]);
                }
            }
        }
    }
}

//==============================================================================
// 6. SERIAL PROTOCOL (Rx Parser & Handshake)
//==============================================================================

void handleCommand (const char* cmd)
{
    // Any valid command from host refreshes the activity watchdog
    lastHostActivity = millis();

    // 1. Auto-Discovery Handshake: Plugin asks "PF_PING?" -> Arduino replies "PF_PONG:..."
    if (strcmp (cmd, "PF_PING?") == 0)
    {
        isConnectedToHost = true;
        Serial.println ("PF_PONG:PulseForge_Controller");

        // Flash 3 LEDs to signal connection established
        for (int t = 0; t < 3; ++t) digitalWrite (LED_PINS[t], HIGH);
        ledOffTime[0] = ledOffTime[1] = ledOffTime[2] = millis() + 180;
        return;
    }

    // 2. Beat / Step LED Flash: "STEP:T0:X" or "STEP:T0:ACC"
    if (strncmp (cmd, "STEP:T", 6) == 0)
    {
        int t = cmd[6] - '0';
        if (t >= 0 && t < 3)
        {
            digitalWrite (LED_PINS[t], HIGH);
            bool isAccent = (strstr (cmd, "ACC") != nullptr);
            ledOffTime[t] = millis() + (isAccent ? 65 : 28);
        }
        return;
    }

    // 3. Update OLED Line: "OLED:T0:L1:text"
    if (strncmp (cmd, "OLED:T", 6) == 0)
    {
        int t = cmd[6] - '0';
        if (t >= 0 && t < 3 && cmd[7] == ':')
        {
            int line = cmd[9] - '0';
            const char* text = &cmd[11];
            if (line == 1) strncpy (oledLine1[t], text, 15);
            else if (line == 2) strncpy (oledLine2[t], text, 15);
            else if (line == 3) strncpy (oledLine3[t], text, 15);
            oledDirty[t] = true;
        }
        return;
    }
}

void processIncomingSerial()
{
    while (Serial.available())
    {
        char c = (char) Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (rxIndex > 0)
            {
                rxBuffer[rxIndex] = '\0';
                handleCommand (rxBuffer);
                rxIndex = 0;
            }
        }
        else if (rxIndex < sizeof (rxBuffer) - 1)
        {
            rxBuffer[rxIndex++] = c;
        }
    }
}

//==============================================================================
// 7. MAIN LOOP
//==============================================================================
void loop()
{
    processIncomingSerial();

    // Only scan hardware inputs (pots, buttons, encoders) AFTER handshake with PulseForge
    if (isConnectedToHost)
    {
        scanButtons();
        scanPotentiometers();
        scanEncoders();

        // Heartbeat timeout: if host closed or disconnected for > 6 seconds, enter standby
        if (millis() - lastHostActivity > 6000)
        {
            isConnectedToHost = false;
        }
    }

    // Turn off LEDs when pulse expires
    const unsigned long now = millis();
    for (int t = 0; t < 3; ++t)
    {
        if (ledOffTime[t] > 0 && now >= ledOffTime[t])
        {
            digitalWrite (LED_PINS[t], LOW);
            ledOffTime[t] = 0;
        }
    }

    // Periodically update OLED displays (round-robin 1 per cycle for smooth frame-rate)
    static int currentOledIndex = 0;
    updateOLED (currentOledIndex);
    currentOledIndex = (currentOledIndex + 1) % 3;
}
