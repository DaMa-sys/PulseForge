/*
 * ==============================================================================
 * Oragen PulseForge - Dedicated Hardware Desktop Controller
 * Target: Arduino Mega 2560 (ATmega2560)
 * Communication: High-Speed Serial @ 115200 Baud with Auto-Discovery Handshake
 *
 * Requirements:
 * - Library "U8g2" by olikraus (install via Arduino Library Manager)
 * ==============================================================================
 */

#include <Arduino.h>

//==============================================================================
// MASCHERE CONFIGURABILI COMPONENTI COLLEGATI (Hardware Connection Masks)
// Se l'Arduino è "a vuoto" o stai montando solo alcuni componenti, metti 'false'
// sui componenti NON ancora collegati.
// I pin contrassegnati come 'false' verranno impostati in INPUT_PULLUP (legati a 5V)
// così NON fluttuano, NON captano ronzio a 50Hz e NON mandano glitch seriali!
// Quando saldi un potenziometro o encoder, basta cambiare il relativo 'false' in 'true'.
//==============================================================================

// Maschera dei 15 Potenziometri [Track 0..2][Param 0..4 (Dens, Swing, BaseOct, ArpOct, Ratio)]
const bool CONNECTED_POTS[3][5] = {
    { true, true, true, true, true }, // Traccia 1 (Bass)
    { true, true, true, true, true }, // Traccia 2 (Arp)
    { true, true, true, true, true }  // Traccia 3 (Accordi)
};

// Maschera dei 6 Pulsanti [Gen, Mut per Track 0..2]
const bool CONNECTED_BUTTONS_GEN[3] = { true, true, true };
const bool CONNECTED_BUTTONS_MUT[3] = { true, true, true };

// Maschera dei 9 Encoder rotativi [Track 0..2][Encoder 0..2 (Genre, Role, Steps)]
const bool CONNECTED_ENCODERS[3][3] = {
    { true, true, true }, // Traccia 1
    { true, true, true }, // Traccia 2
    { true, true, true }  // Traccia 3
};

// Maschera dei 3 LED di stato (PWM Hardware su pin 8, 9, 10)
const bool CONNECTED_LEDS[3] = { true, true, true };

// Tabelle di Inversione Software (true = inverte verso di rotazione senza toccare i fili!)
// Ordine Pot: [0]=Density, [1]=Swing, [2]=BaseOct, [3]=ArpOct, [4]=Ratio
const bool INVERT_POT[3][5] = {
    { false, false, false, true,  false }, // Ch 1 (cablato su A10..A14): solo ArpOct invertito
    { true,  true,  true,  true,  false }, // Ch 2 (A5..A9): Dens, Swing, BaseOct, ArpOct invertiti
    { true,  true,  true,  true,  false }  // Ch 3 (cablato su A0..A4): Dens, Swing, BaseOct, ArpOct invertiti
};

// Ordine Encoder: [0]=Genre, [1]=Role, [2]=Steps
const bool INVERT_ENC[3][3] = {
    { false, false, true }, // Ch 1 (cablato su 34..39): Steps invertito
    { false, false, true }, // Ch 2 (28..33): Steps invertito
    { false, false, true }  // Ch 3 (cablato su 22..27): Steps invertito
};

// Maschera dei 3 Display OLED [Track 0..2] e canali TCA9548A
const bool CONNECTED_OLEDS[3] = { true, true, true }; // Tutti e 3 gli OLED ATTIVI tramite TCA9548A!
const uint8_t OLED_MUX[3]     = { 2, 1, 0 };          // Canali I2C del multiplexer TCA9548A per Track 1, 2, 3

#define ENABLE_OLED
#ifdef ENABLE_OLED
  #include <Wire.h>
  #include <U8g2lib.h>
  #define TCA9548A_ADDR 0x70

  inline void tcaselect (uint8_t channel)
  {
      if (channel > 7) return;
      Wire.beginTransmission (TCA9548A_ADDR);
      Wire.write (1 << channel);
      Wire.endTransmission();
  }
#endif

//==============================================================================
// 1. PIN DEFINITIONS
//==============================================================================

// 15 Analog Inputs (Direct to Arduino Mega ADC - CH 1 e CH 3 scambiati per cablaggio dal retro)
const uint8_t POT_PINS[3][5] = {
    { A10, A11, A12, A13, A14 },  // Track 1 (Bass):   Density, Swing, BaseOct, ArpOct, Ratio
    { A5,  A6,  A7,  A8,  A9  },  // Track 2 (Arp):    Density, Swing, BaseOct, ArpOct, Ratio
    { A0,  A1,  A2,  A3,  A4  }   // Track 3 (Chords): Density, Swing, BaseOct, ArpOct, Ratio
};

// 6 Tactile Buttons (INPUT_PULLUP)
const uint8_t BTN_GEN[3] = { 6, 4, 2 };
const uint8_t BTN_MUT[3] = { 7, 5, 3 };

// 3 Beat / Step Indicator LEDs
const uint8_t LED_PINS[3] = { 10, 9, 8 };

// 3 OLED Displays: Tutti su Fast Hardware I2C (Pin 20 SDA, Pin 21 SCL @ 400kHz) tramite TCA9548A!
// Un'unica istanza U8g2 condivisa risparmia 2KB di SRAM e seleziona il canale con tcaselect(t)
#ifdef ENABLE_OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled (U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

// 9 Rotary Encoders (CLK, DT - No Click, solo 2 fili di segnale!)
struct EncoderPins {
    uint8_t pinCLK;
    uint8_t pinDT;
};

const EncoderPins ENCODERS[3][3] = {
    // Track 1: Genre, Role, Steps
    { { 34, 35 }, { 36, 37 }, { 38, 39 } },
    // Track 2: Genre, Role, Steps
    { { 28, 29 }, { 30, 31 }, { 32, 33 } },
    // Track 3: Genre, Role, Steps
    { { 22, 23 }, { 24, 25 }, { 26, 27 } }
};

//==============================================================================
// 2. STATE & BUFFERING (Anti-jitter & Debounce)
//==============================================================================

// Potentiometer readings (7-bit MIDI-like scale 0..127 with 8x oversampling)
int lastCoarseVal[3][5] = {
    { -99, -99, -99, -99, -99 },
    { -99, -99, -99, -99, -99 },
    { -99, -99, -99, -99, -99 }
};
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

// Encoder quadrature state machine (1kHz Timer3 ISR with direct port reads: zero missed steps!)
volatile uint8_t* encPortCLK[3][3];
volatile uint8_t* encPortDT[3][3];
uint8_t encMaskCLK[3][3];
uint8_t encMaskDT[3][3];
volatile uint8_t lastEncoderBitsISR[3][3];
volatile int8_t encoderAccumulatorISR[3][3];
volatile int8_t encPendingDelta[3][3];

ISR (TIMER3_COMPA_vect)
{
    static const int8_t qTable[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    for (uint8_t t = 0; t < 3; ++t)
    {
        for (uint8_t e = 0; e < 3; ++e)
        {
            if (!CONNECTED_ENCODERS[t][e])
                continue;

            uint8_t a = (*encPortCLK[t][e] & encMaskCLK[t][e]) ? 1 : 0;
            uint8_t b = (*encPortDT[t][e]  & encMaskDT[t][e])  ? 1 : 0;
            uint8_t curr = (a << 1) | b;

            if (curr != lastEncoderBitsISR[t][e])
            {
                int8_t step = qTable[(lastEncoderBitsISR[t][e] << 2) | curr];
                encoderAccumulatorISR[t][e] += step;
                lastEncoderBitsISR[t][e] = curr;

                if (curr == 3)
                {
                    if (encoderAccumulatorISR[t][e] > 0)      encPendingDelta[t][e]++;
                    else if (encoderAccumulatorISR[t][e] < 0) encPendingDelta[t][e]--;
                    encoderAccumulatorISR[t][e] = 0;
                }
            }
        }
    }
}

// LED PWM smooth fade state (0..255)
uint8_t ledBrightness[3] = { 0, 0, 0 };
unsigned long lastLedFadeTime = 0;

inline void triggerLedFlash (int t, uint8_t level)
{
    if (t >= 0 && t < 3 && CONNECTED_LEDS[t])
    {
        ledBrightness[t] = level;
        analogWrite (LED_PINS[t], level);
    }
}

// OLED Studio HUD telemetry
const char* const GENRE_NAMES[8] = {
    "CLASSIC HOUSE",
    "ACID 303",
    "ITALO DISCO",
    "FISHER",
    "P. KALKBRENNER",
    "UK GARAGE",
    "FRED AGAIN..",
    "BONOBO"
};
int currentGenreIdx[3] = { 0, 1, 2 };

const char* const ROLE_NAMES[7] = {
    "Mono Bassline",
    "Acid TB-303",
    "Classic Arp",
    "Lead Hook",
    "Poly 7th Stab",
    "Harmony Chords",
    "Modular Ratchet"
};
int currentRoleIdx[3] = { 0, 1, 2 };

const char* const RATIO_NAMES[11] = {
    "1/4x", "1/3x", "1/2x", "2/3x", "3/4x",
    "1/1x",
    "4/3x", "3/2x", "2/1x", "3/1x", "4/1x"
};
const int STEP_CHOICES[4] = { 4, 8, 16, 32 };

char trackName[3][18]  = { "DAW",           "DAW",           "DAW" };
char trackGenre[3][16] = { "CLASSIC HOUSE", "ACID 303",      "CLASSIC HOUSE" };
char trackRole[3][16]  = { "Mono Bassline", "Rolling Arp",   "Poly 7th Stab" };
int trackSteps[3]      = { 16, 16, 32 };
int trackBaseOct[3]    = { 1, 2, 3 };    // C1, C2, C3
int trackArpOct[3]     = { 2, 3, 1 };    // +2, +3, +1
int trackDensity[3]    = { 65, 80, 50 }; // 0..100%
int trackSwing[3]      = { 16, 25, 0 };  // 0..75%
char trackRatio[3][8]  = { "1/1x", "1/2x", "2/1x" };

// Parameter Pop-up HUD state
unsigned long popupUntil[3] = { 0, 0, 0 };
char popupTitle[3][16];
char popupVal[3][16];
int popupPercent[3] = { 0, 0, 0 };
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

#ifdef ENABLE_OLED
    Wire.begin();
    Wire.setClock (400000L); // 400kHz Fast Hardware I2C!
#endif

    // Initialize Buttons, LEDs & Potentiometers with hardware mask protection
    for (int t = 0; t < 3; ++t)
    {
        if (CONNECTED_BUTTONS_GEN[t]) pinMode (BTN_GEN[t], INPUT_PULLUP);
        else                          pinMode (BTN_GEN[t], INPUT_PULLUP);

        if (CONNECTED_BUTTONS_MUT[t]) pinMode (BTN_MUT[t], INPUT_PULLUP);
        else                          pinMode (BTN_MUT[t], INPUT_PULLUP);

        if (CONNECTED_LEDS[t])
        {
            pinMode (LED_PINS[t], OUTPUT);
            digitalWrite (LED_PINS[t], LOW);
        }

        for (int p = 0; p < 5; ++p)
        {
            if (CONNECTED_POTS[t][p])
            {
                // Connected potentiometer: standard analog input
                pinMode (POT_PINS[t][p], INPUT);
                int initCoarse = analogRead (POT_PINS[t][p]) >> 3;
                if (INVERT_POT[t][p]) initCoarse = 127 - initCoarse;
                lastCoarseVal[t][p] = initCoarse;
            }
            else
            {
                // Unconnected/floating pin: tie to 5V via internal pullup
                pinMode (POT_PINS[t][p], INPUT_PULLUP);
                lastCoarseVal[t][p] = 127;
            }
        }

        for (int e = 0; e < 3; ++e)
        {
            uint8_t pCLK = ENCODERS[t][e].pinCLK;
            uint8_t pDT  = ENCODERS[t][e].pinDT;
            pinMode (pCLK, INPUT_PULLUP);
            pinMode (pDT,  INPUT_PULLUP);

            encPortCLK[t][e] = portInputRegister (digitalPinToPort (pCLK));
            encMaskCLK[t][e] = digitalPinToBitMask (pCLK);
            encPortDT[t][e]  = portInputRegister (digitalPinToPort (pDT));
            encMaskDT[t][e]  = digitalPinToBitMask (pDT);

            uint8_t a = (*encPortCLK[t][e] & encMaskCLK[t][e]) ? 1 : 0;
            uint8_t b = (*encPortDT[t][e]  & encMaskDT[t][e])  ? 1 : 0;
            lastEncoderBitsISR[t][e]    = (a << 1) | b;
            encoderAccumulatorISR[t][e] = 0;
            encPendingDelta[t][e]       = 0;
        }

        // Initialize OLED via TCA9548A channel t
#ifdef ENABLE_OLED
        if (CONNECTED_OLEDS[t])
        {
            tcaselect (OLED_MUX[t]);
            oled.setBusClock (400000L);
            oled.begin();
        }
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

    // Configure Timer3 for 1 kHz hardware quadrature encoder polling (zero missed steps)
    cli();
    TCCR3A = 0;
    TCCR3B = 0;
    TCNT3  = 0;
    OCR3A  = 249;                        // 16 MHz / (64 * 1000 Hz) - 1 = 249
    TCCR3B |= (1 << WGM32);              // CTC mode
    TCCR3B |= (1 << CS31) | (1 << CS30); // Prescaler 64
    TIMSK3 |= (1 << OCIE3A);             // Enable Timer3 Compare A interrupt
    sei();
}

//==============================================================================
// 4. OLED RENDERING (Yellow-Blue Dual Color Layout: Fast & Big Fonts)
//==============================================================================
void updateOLED (int t)
{
#ifdef ENABLE_OLED
    if (!CONNECTED_OLEDS[t] || !oledDirty[t])
        return;

    const bool showPopup = (millis() < popupUntil[t]);

    oled.clearBuffer();

    if (showPopup)
    {
        // =====================================================================
        // POP-UP VIEW (Quando tocchi una manopola)
        // =====================================================================
        // 1. ZONA GIALLA SUPERIORE (y = 0..15): Titolo Funzione in Giallo
        oled.setFont (u8g2_font_7x14B_tf);
        int tw = oled.getStrWidth (popupTitle[t]);
        oled.drawStr ((128 - tw) / 2, 12, popupTitle[t]);
        oled.drawHLine (0, 15, 128); // Divisione netta tra giallo e blu

        // 2. ZONA BLU INFERIORE (y = 16..63): Testo Centrato che ENTRA SEMPRE PERFETTAMENTE
        if (strlen (popupVal[t]) <= 6)
        {
            // Per valori brevi tipo "68 %", "Oct C1" (Font bello e leggibile)
            oled.setFont (u8g2_font_helvB18_tf);
            int vw = oled.getStrWidth (popupVal[t]);
            oled.drawStr ((128 - vw) / 2, 43, popupVal[t]);
        }
        else
        {
            // Per stringhe lunghe (es. "CLASSIC HOUSE", "BERLIN SCHOOL", "EUCLIDEAN IDM")
            // u8g2_font_8x13B_tf entra SEMPRE con 12-16px di margine, centrato al millimetro!
            oled.setFont (u8g2_font_8x13B_tf);
            int vw = oled.getStrWidth (popupVal[t]);
            oled.drawStr ((128 - vw) / 2, 41, popupVal[t]);
        }

        // Barra Grafica orizzontale dinamica (y = 51..58)
        oled.drawFrame (10, 51, 108, 8);
        int fillW = (popupPercent[t] * 104) / 100;
        if (fillW > 104) fillW = 104;
        if (fillW < 0) fillW = 0;
        if (fillW > 0) oled.drawBox (12, 53, fillW, 4);
    }
    else
    {
        // =====================================================================
        // DASHBOARD RESTING VIEW (A riposo: Spazi Larghi, Grandi e Leggibili)
        // =====================================================================
        // 1. INTESTAZIONE SUPERIORE (y = 0..19): Nome Dispositivo grande + Step a destra
        char stpStr[8];
        snprintf (stpStr, sizeof (stpStr), "%dst", trackSteps[t]);
        oled.setFont (u8g2_font_7x14B_tf);
        int sw = oled.getStrWidth (stpStr);
        int maxNameW = 128 - sw - 5; // Limite pixel a destra per non sovrapporsi allo step

        // Scala automaticamente dal font più grande e leggibile a quelli più compatti se il nome è lungo
        oled.setFont (u8g2_font_helvB14_tf);
        if (oled.getStrWidth (trackName[t]) > maxNameW)
            oled.setFont (u8g2_font_9x15B_tf);
        if (oled.getStrWidth (trackName[t]) > maxNameW)
            oled.setFont (u8g2_font_7x14B_tf);
        if (oled.getStrWidth (trackName[t]) > maxNameW)
            oled.setFont (u8g2_font_6x12_tf);

        oled.drawStr (0, 16, trackName[t]);

        // Disegna sempre lo step a destra in grassetto 7x14B
        oled.setFont (u8g2_font_7x14B_tf);
        oled.drawStr (128 - sw, 14, stpStr);
        oled.drawHLine (0, 19, 128); // Divisione netta sotto l'intestazione

        // 2. ZONA INFERIORE (y = 20..63):
        // Riga 1: GENERE centrato e largo (y = 38)
        oled.setFont (u8g2_font_8x13B_tf);
        int gw = oled.getStrWidth (trackGenre[t]);
        oled.drawStr ((128 - gw) / 2, 38, trackGenre[t]);

        // Riga 2: RUOLO STRUMENTO centrato (y = 56)
        oled.setFont (u8g2_font_7x14B_tf);
        int rw = oled.getStrWidth (trackRole[t]);
        oled.drawStr ((128 - rw) / 2, 56, trackRole[t]);

        // Linea minimale sul fondo
        oled.drawHLine (0, 62, 128);
    }

    tcaselect (OLED_MUX[t]);
    oled.sendBuffer();
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
        if (CONNECTED_BUTTONS_GEN[t])
        {
            int gVal = digitalRead (BTN_GEN[t]);
            if (gVal != lastBtnGenState[t] && (now - lastBtnDebounceTime[t] > 50))
            {
                lastBtnDebounceTime[t] = now;
                lastBtnGenState[t] = gVal;
                if (gVal == LOW) // Pressed
                {
                    Serial.print ("T"); Serial.print (t); Serial.println (":GEN");
                    triggerLedFlash (t, 90);
                }
            }
        }

        // 2. MUTATE Button
        if (CONNECTED_BUTTONS_MUT[t])
        {
            int mVal = digitalRead (BTN_MUT[t]);
            if (mVal != lastBtnMutState[t] && (now - lastBtnDebounceTime[t] > 50))
            {
                lastBtnDebounceTime[t] = now;
                lastBtnMutState[t] = mVal;
                if (mVal == LOW) // Pressed
                {
                    Serial.print ("T"); Serial.print (t); Serial.println (":MUT");
                    triggerLedFlash (t, 90);
                }
            }
        }
    }
}

void scanPotentiometers()
{
    const unsigned long now = millis();
    if (now - lastPotReadTime < 2) // Round-robin: 1 pot every 2 ms (full 15-pot cycle = 30 ms, ~1 ms max block)
        return;
    lastPotReadTime = now;

    static const char* potNames[5] = { "DENS", "SWING", "BOCT", "AOCT", "RATIO" };
    static uint8_t rrTrack = 0;
    static uint8_t rrPot   = 0;

    int t = rrTrack;
    int p = rrPot;

    // Advance round-robin indices for next call
    if (++rrPot >= 5)
    {
        rrPot = 0;
        if (++rrTrack >= 3)
            rrTrack = 0;
    }

    if (!CONNECTED_POTS[t][p])
        return;

    // 1. Channel settle & 8x Oversampling to kill all high-frequency USB noise
    analogRead (POT_PINS[t][p]); // discard first read on mux switch
    long sum = 0;
    for (int i = 0; i < 8; ++i)
    {
        sum += analogRead (POT_PINS[t][p]);
    }
    int raw = (int) (sum >> 3); // 10-bit average (0..1023)

    // 2. Truncate bottom 3 noisy bits to get a rock-solid 7-bit scale (0..127)
    int coarse = raw >> 3;
    if (INVERT_POT[t][p])
        coarse = 127 - coarse;

    // 3. Strict Hysteresis: only trigger if coarse change is >= 2 steps (eliminates 100% false jitter!)
    if (abs (coarse - lastCoarseVal[t][p]) >= 2)
    {
        lastCoarseVal[t][p] = coarse;

                int val = 0;
                if (p == 0)      val = map (coarse, 0, 127, 0, 100);  // Density 0..100%
                else if (p == 1) val = map (coarse, 0, 127, 0, 75);   // Swing 0..75%
                else if (p == 2) val = map (coarse, 0, 127, 0, 5);    // Base Oct 0..5
                else if (p == 3) val = map (coarse, 0, 127, 1, 4);    // Arp Oct 1..4
                else if (p == 4) val = map (coarse, 0, 127, 0, 10);   // Tempo Ratio 0..10

                // Update track state and trigger dynamic Pop-up HUD card
                popupUntil[t] = millis() + 1200;
                if (p == 0)
                {
                    trackDensity[t] = val;
                    strncpy (popupTitle[t], "DENSITY", sizeof (popupTitle[t]));
                    snprintf (popupVal[t], sizeof (popupVal[t]), "%d %%", val);
                    popupPercent[t] = val;
                }
                else if (p == 1)
                {
                    trackSwing[t] = val;
                    strncpy (popupTitle[t], "SWING", sizeof (popupTitle[t]));
                    snprintf (popupVal[t], sizeof (popupVal[t]), "%d %%", val);
                    popupPercent[t] = (val * 100) / 75;
                }
                else if (p == 2)
                {
                    trackBaseOct[t] = val;
                    strncpy (popupTitle[t], "BASE OCT", sizeof (popupTitle[t]));
                    snprintf (popupVal[t], sizeof (popupVal[t]), "Oct C%d", val);
                    popupPercent[t] = (val * 100) / 5;
                }
                else if (p == 3)
                {
                    trackArpOct[t] = val;
                    strncpy (popupTitle[t], "ARP OCT", sizeof (popupTitle[t]));
                    snprintf (popupVal[t], sizeof (popupVal[t]), "+%d Oct", val);
                    popupPercent[t] = (val * 100) / 4;
                }
                else if (p == 4)
                {
                    int rIdx = constrain (val, 0, 10);
                    strncpy (trackRatio[t], RATIO_NAMES[rIdx], sizeof (trackRatio[t]));
                    strncpy (popupTitle[t], "RATIO", sizeof (popupTitle[t]));
                    snprintf (popupVal[t], sizeof (popupVal[t]), "%s", RATIO_NAMES[rIdx]);
                    popupPercent[t] = (rIdx * 100) / 10;
                }
                oledDirty[t] = true;

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

void scanEncoders()
{
    static const char* encNames[3] = { "GENRE", "ROLE", "STEPS" };

    for (int t = 0; t < 3; ++t)
    {
        for (int e = 0; e < 3; ++e)
        {
            if (!CONNECTED_ENCODERS[t][e])
                continue;

            int8_t delta = 0;
            cli();
            delta = encPendingDelta[t][e];
            encPendingDelta[t][e] = 0;
            sei();

            if (delta == 0)
                continue;

            if (INVERT_ENC[t][e])
                delta = -delta;

            int8_t stepDir = (delta > 0) ? 1 : -1;
            uint8_t count  = (delta > 0) ? (uint8_t) delta : (uint8_t) (-delta);

            for (uint8_t s = 0; s < count; ++s)
            {
                // Send Serial command to host
                Serial.print ("T"); Serial.print (t);
                Serial.print (":ENC:"); Serial.print (encNames[e]);
                Serial.print (":"); Serial.println (stepDir > 0 ? "+1" : "-1");

                // Aggiornamento istantaneo e diretto sulla Home (clamping 1:1 con il plugin, niente wrap-around)
                if (e == 0) // GENRE
                {
                    currentGenreIdx[t] = constrain (currentGenreIdx[t] + stepDir, 0, 7);
                    strncpy (trackGenre[t], GENRE_NAMES[currentGenreIdx[t]], sizeof (trackGenre[t]));
                }
                else if (e == 1) // ROLE
                {
                    currentRoleIdx[t] = constrain (currentRoleIdx[t] + stepDir, 0, 6);
                    strncpy (trackRole[t], ROLE_NAMES[currentRoleIdx[t]], sizeof (trackRole[t]));
                }
                else if (e == 2) // STEPS (solo 4, 8, 16, 32 come nel plugin)
                {
                    int stepIdx = 2; // default 16
                    for (int k = 0; k < 4; ++k)
                    {
                        if (trackSteps[t] == STEP_CHOICES[k])
                        {
                            stepIdx = k;
                            break;
                        }
                    }
                    stepIdx = constrain (stepIdx + stepDir, 0, 3);
                    trackSteps[t] = STEP_CHOICES[stepIdx];
                }
            }
            oledDirty[t] = true;
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

        // Flash 3 LEDs softly to signal connection established
        for (int t = 0; t < 3; ++t) triggerLedFlash (t, 120);
        return;
    }

    // 2. Beat / Step LED Flash: "STEP:T0:X" or "STEP:T0:ACC"
    if (strncmp (cmd, "STEP:T", 6) == 0)
    {
        int t = cmd[6] - '0';
        if (t >= 0 && t < 3)
        {
            bool isAccent = (strstr (cmd, "ACC") != nullptr);
            triggerLedFlash (t, isAccent ? 150 : 48);
        }
        return;
    }

    // 3. Update OLED telemetry from host plugin:
    if (strncmp (cmd, "OLED:T", 6) == 0)
    {
        int t = cmd[6] - '0';
        if (t >= 0 && t < 3 && cmd[7] == ':')
        {
            const char* sub = &cmd[8];
            if (strncmp (sub, "NAME:", 5) == 0)
            {
                strncpy (trackName[t], sub + 5, sizeof (trackName[t]) - 1);
                trackName[t][sizeof (trackName[t]) - 1] = '\0';
            }
            else if (strncmp (sub, "GEN:", 4) == 0)
            {
                strncpy (trackGenre[t], sub + 4, sizeof (trackGenre[t]) - 1);
                trackGenre[t][sizeof (trackGenre[t]) - 1] = '\0';
                for (int k = 0; k < 8; ++k)
                {
                    if (strcmp (trackGenre[t], GENRE_NAMES[k]) == 0)
                    {
                        currentGenreIdx[t] = k;
                        break;
                    }
                }
            }
            else if (strncmp (sub, "ROL:", 4) == 0)
            {
                strncpy (trackRole[t], sub + 4, sizeof (trackRole[t]) - 1);
                trackRole[t][sizeof (trackRole[t]) - 1] = '\0';
                for (int k = 0; k < 7; ++k)
                {
                    if (strcmp (trackRole[t], ROLE_NAMES[k]) == 0)
                    {
                        currentRoleIdx[t] = k;
                        break;
                    }
                }
            }
            else if (strncmp (sub, "STP:", 4) == 0)
            {
                trackSteps[t] = atoi (sub + 4);
            }
            else if (strncmp (sub, "OCT:", 4) == 0)
            {
                trackBaseOct[t] = atoi (sub + 4);
            }
            else if (strncmp (sub, "BOCT:", 5) == 0)
            {
                int val = atoi (sub + 5);
                trackBaseOct[t] = val;
                lastSentPotVal[t][2] = val;
                popupUntil[t] = millis() + 1200;
                strncpy (popupTitle[t], "BASE OCT", sizeof (popupTitle[t]));
                snprintf (popupVal[t], sizeof (popupVal[t]), "Oct C%d", val);
                popupPercent[t] = (val * 100) / 5;
            }
            else if (strncmp (sub, "AOCT:", 5) == 0)
            {
                int val = atoi (sub + 5);
                trackArpOct[t] = val;
                lastSentPotVal[t][3] = val;
                popupUntil[t] = millis() + 1200;
                strncpy (popupTitle[t], "ARP OCT", sizeof (popupTitle[t]));
                snprintf (popupVal[t], sizeof (popupVal[t]), "+%d Oct", val);
                popupPercent[t] = (val * 100) / 4;
            }
            else if (strncmp (sub, "RATIDX:", 7) == 0)
            {
                int rIdx = constrain (atoi (sub + 7), 0, 10);
                strncpy (trackRatio[t], RATIO_NAMES[rIdx], sizeof (trackRatio[t]));
                lastSentPotVal[t][4] = rIdx;
                popupUntil[t] = millis() + 1200;
                strncpy (popupTitle[t], "RATIO", sizeof (popupTitle[t]));
                snprintf (popupVal[t], sizeof (popupVal[t]), "%s", RATIO_NAMES[rIdx]);
                popupPercent[t] = (rIdx * 100) / 10;
            }
            else if (strncmp (sub, "RAT:", 4) == 0)
            {
                strncpy (trackRatio[t], sub + 4, sizeof (trackRatio[t]) - 1);
                trackRatio[t][sizeof (trackRatio[t]) - 1] = '\0';
            }
            else if (strncmp (sub, "DENS:", 5) == 0)
            {
                int val = atoi (sub + 5);
                trackDensity[t] = val;
                lastSentPotVal[t][0] = val;
                popupUntil[t] = millis() + 1200;
                strncpy (popupTitle[t], "DENSITY", sizeof (popupTitle[t]));
                snprintf (popupVal[t], sizeof (popupVal[t]), "%d %%", val);
                popupPercent[t] = val;
            }
            else if (strncmp (sub, "SWING:", 6) == 0)
            {
                int val = atoi (sub + 6);
                trackSwing[t] = val;
                lastSentPotVal[t][1] = val;
                popupUntil[t] = millis() + 1200;
                strncpy (popupTitle[t], "SWING", sizeof (popupTitle[t]));
                snprintf (popupVal[t], sizeof (popupVal[t]), "%d %%", val);
                popupPercent[t] = (val * 100) / 75;
            }

            oledDirty[t] = true; // Ridipinge all'istante lo schermino per qualsiasi comando ricevuto dal PC!
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

    // Hardware inputs scan (always updates OLED; sends serial when host connected)
    scanButtons();
    scanPotentiometers();
    scanEncoders();


    // Smooth PWM fade-out for Beat/Step LEDs (~80Hz, non-blocking)
    const unsigned long now = millis();
    if (now - lastLedFadeTime >= 12)
    {
        lastLedFadeTime = now;
        for (int t = 0; t < 3; ++t)
        {
            if (CONNECTED_LEDS[t] && ledBrightness[t] > 0)
            {
                uint8_t nextVal = (ledBrightness[t] > 6) ? (ledBrightness[t] - (ledBrightness[t] >> 2) - 2) : 0;
                ledBrightness[t] = nextVal;
                analogWrite (LED_PINS[t], nextVal);
            }
        }
    }

    // Restore full studio dashboard when editing pop-up expires
    for (int t = 0; t < 3; ++t)
    {
        if (popupUntil[t] > 0 && millis() >= popupUntil[t])
        {
            popupUntil[t] = 0;
            oledDirty[t] = true;
        }
    }

    // Update active OLED displays (throttled to 25 FPS so ADC scanning and pot responsiveness are instantaneous!)
    static unsigned long lastOledFrameTime = 0;
    if (now - lastOledFrameTime >= 35)
    {
        lastOledFrameTime = now;
        static int currentOledIndex = 0;
        if (CONNECTED_OLEDS[currentOledIndex] && oledDirty[currentOledIndex])
        {
            updateOLED (currentOledIndex);
        }
        currentOledIndex = (currentOledIndex + 1) % 3;
    }
}
