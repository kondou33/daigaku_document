#include "EntertainmentRobot.h"

// ==========================================
// 全休符テーブルの定義 (false: 演奏, true: 全休符)
// ==========================================
const bool EntertainmentRobot::WHOLE_REST_BAR_TABLE[SERVER_BAR_COUNT] = {
  false, false, false, false, false, false, false, false, // 0〜7小節 (1周目)
  true,  true,  true,  true,  true,                       // 8〜12小節 (休符)
  false, false, false, false, false, false, false, false, // 13〜20小節 (2周目)
  true,  true,  true,  true                               // 21〜24小節 (休符)
};

// ==========================================
// 楽譜テーブルの定義 (25小節分)
// ==========================================
#define SCORE_FILL         { SCORE_END, 0 }

#define BAR_QUARTER_4      { \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_PLAY, LENGTH_QUARTER}, \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_PLAY, LENGTH_QUARTER}, \
  SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL \
}

#define BAR_QUARTER_3_REST_1 { \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_PLAY, LENGTH_QUARTER}, \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_REST, LENGTH_QUARTER}, \
  SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL \
}

#define BAR_EIGHTH_8       { \
  {SCORE_PLAY, LENGTH_EIGHTH}, {SCORE_PLAY, LENGTH_EIGHTH}, \
  {SCORE_PLAY, LENGTH_EIGHTH}, {SCORE_PLAY, LENGTH_EIGHTH}, \
  {SCORE_PLAY, LENGTH_EIGHTH}, {SCORE_PLAY, LENGTH_EIGHTH}, \
  {SCORE_PLAY, LENGTH_EIGHTH}, {SCORE_PLAY, LENGTH_EIGHTH} \
}

#define BAR_WHOLE_REST     { \
  {SCORE_REST, LENGTH_WHOLE}, \
  SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL \
}

#define BAR_NOTE_REST_NOTE_REST { \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_REST, LENGTH_QUARTER}, \
  {SCORE_PLAY, LENGTH_QUARTER}, {SCORE_REST, LENGTH_QUARTER}, \
  SCORE_FILL, SCORE_FILL, SCORE_FILL, SCORE_FILL \
}

const ServoScoreEvent EntertainmentRobot::SCORE_TABLE[SERVER_BAR_COUNT][MAX_EVENTS_PER_BAR] = {
  BAR_QUARTER_4,            // 0小節目
  BAR_QUARTER_3_REST_1,     // 1小節目
  BAR_QUARTER_4,            // 2小節目
  BAR_QUARTER_3_REST_1,     // 3小節目
  BAR_NOTE_REST_NOTE_REST,  // 4小節目
  BAR_NOTE_REST_NOTE_REST,  // 5小節目
  BAR_EIGHTH_8,             // 6小節目
  BAR_QUARTER_3_REST_1,     // 7小節目
  
  BAR_WHOLE_REST,           // 8小節目
  BAR_WHOLE_REST,           // 9小節目
  BAR_WHOLE_REST,           // 10小節目
  BAR_WHOLE_REST,           // 11小節目
  BAR_WHOLE_REST,           // 12小節目

  BAR_QUARTER_4,            // 13小節目
  BAR_QUARTER_3_REST_1,     // 14小節目
  BAR_QUARTER_4,            // 15小節目
  BAR_QUARTER_3_REST_1,     // 16小節目
  BAR_NOTE_REST_NOTE_REST,  // 17小節目
  BAR_NOTE_REST_NOTE_REST,  // 18小節目
  BAR_EIGHTH_8,             // 19小節目
  BAR_QUARTER_3_REST_1,     // 20小節目

  BAR_WHOLE_REST,           // 21小節目
  BAR_WHOLE_REST,           // 22小節目
  BAR_WHOLE_REST,           // 23小節目
  BAR_WHOLE_REST            // 24小節目
};

#undef SCORE_FILL
#undef BAR_QUARTER_4
#undef BAR_QUARTER_3_REST_1
#undef BAR_EIGHTH_8
#undef BAR_WHOLE_REST
#undef BAR_NOTE_REST_NOTE_REST


EntertainmentRobot::EntertainmentRobot() :
    _currentBPM(120), _currentBar(0), _nextEventTime(0),
    _scoreEventIndex(0), _usedUnitsInBar(0),
    _barActive(false), _armDirection(false), _motionMutedByWholeRest(false),
    _scoreRestActive(true), _pianoUseRightArm(true),
    _rightArmReturnActive(false), _leftArmReturnActive(false), _extraReturnActive(false),
    _rightArmReturnTime(0), _leftArmReturnTime(0), _extraReturnTime(0),
    _emotionActive(false), _lastEmotionMoveTime(0), _emotionDirection(false)
{
    calculateBeatInterval();
}

void EntertainmentRobot::begin(const char* ssid, const char* pass, uint16_t port) {
    _localPort = port;
    
    setupServos();
    calculateBeatInterval();
    
    Serial.println("Connecting to Wi-Fi...");
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWi-Fi Connected!");

    _udp.begin(_localPort);

    Serial.println("Entertainment client started");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void EntertainmentRobot::update() {
    receiveUdpData();
    updateNormalMotion();
    updateEmotionMotion();
    updateServoReturn();
}

void EntertainmentRobot::setupServos() {
#if INSTRUMENT == PIANO
    _rightArmServo.attach(RIGHT_ARM_PIN);
    _leftArmServo.attach(LEFT_ARM_PIN);
    _rightArmServo.write(ARM_REST_ANGLE);
    _leftArmServo.write(ARM_REST_ANGLE);

#elif INSTRUMENT == TROMBONE
    _rightArmServo.attach(RIGHT_ARM_PIN);
    _rightArmServo.write(ARM_REST_ANGLE);

#elif INSTRUMENT == VIOLIN
    _rightArmServo.attach(RIGHT_ARM_PIN);
    _extraServo.attach(EXTRA_PIN);
    _rightArmServo.write(ARM_REST_ANGLE);
    _extraServo.write(HEAD_CENTER_ANGLE);

#elif INSTRUMENT == CASTANET
    _rightArmServo.attach(RIGHT_ARM_PIN);
    _extraServo.attach(EXTRA_PIN);
    _rightArmServo.write(ARM_REST_ANGLE);
    _extraServo.write(MOUTH_CLOSE_ANGLE);
#endif
}

void EntertainmentRobot::receiveUdpData() {
    int packetSize = _udp.parsePacket();

    if (packetSize > 0) {
        int data = _udp.read();

        while (_udp.available()) {
            _udp.read();
        }

        if (data < 0) {
            return;
        }

        // 40以上（または25以上）をBPMとして判定するための境界
        if (data >= 40) {
            updateBPM((uint8_t)data);
        } else {
            startBarMotion((uint8_t)data);
        }
    }
}

void EntertainmentRobot::updateBPM(uint8_t newBPM) {
    if (newBPM == _currentBPM) {
        return;
    }

    _currentBPM = newBPM;
    calculateBeatInterval();

    Serial.print("BPM updated: ");
    Serial.println(_currentBPM);

    startEmotion();
}

void EntertainmentRobot::calculateBeatInterval() {
    _beatInterval = 60000UL / _currentBPM;
}

unsigned long EntertainmentRobot::noteLengthToMs(uint8_t lengthUnits) {
    if (lengthUnits == 0) return 0;
    return (_beatInterval * (unsigned long)lengthUnits) / QUARTER_LENGTH_UNITS;
}

unsigned long EntertainmentRobot::calculatePulseTime(unsigned long eventDuration) {
    if (eventDuration == 0) return 0;
    unsigned long pulseTime = (eventDuration * 7UL) / 10UL;

    if (pulseTime < 40UL) { pulseTime = 40UL; }
    if (eventDuration > 30UL && pulseTime > eventDuration - 20UL) {
        pulseTime = eventDuration - 20UL;
    }
    if (pulseTime > eventDuration) { pulseTime = eventDuration; }

    return pulseTime;
}

uint8_t EntertainmentRobot::toScoreBar(uint8_t serverBarNumber) {
    int scoreBar = (int)serverBarNumber + PART_BAR_OFFSET;
    scoreBar %= SERVER_BAR_COUNT;

    if (scoreBar < 0) {
        scoreBar += SERVER_BAR_COUNT;
    }

    return (uint8_t)scoreBar;
}

bool EntertainmentRobot::isWholeRestBar(uint8_t scoreBarNumber) {
    if (scoreBarNumber >= SERVER_BAR_COUNT) {
        return false;
    }
    return WHOLE_REST_BAR_TABLE[scoreBarNumber];
}

void EntertainmentRobot::startBarMotion(uint8_t barNumber) {
    uint8_t scoreBar = toScoreBar(barNumber);
    _currentBar = scoreBar;

    Serial.print("Server bar: ");
    Serial.print(barNumber);
    Serial.print(" -> Score bar: ");
    Serial.println(_currentBar);

    if (isWholeRestBar(_currentBar)) {
        _motionMutedByWholeRest = true;
        _scoreRestActive = true;
        stopAllMotionForWholeRest();
        return;
    }

    _motionMutedByWholeRest = false;
    _scoreRestActive = false;

    _nextEventTime = millis();
    _scoreEventIndex = 0;
    _usedUnitsInBar = 0;
    _barActive = true;
}

void EntertainmentRobot::updateNormalMotion() {
    if (_motionMutedByWholeRest || !_barActive) {
        return;
    }

    while (_barActive && timeReached(_nextEventTime)) {
        triggerNextScoreEvent();
    }
}

void EntertainmentRobot::triggerNextScoreEvent() {
    if (_scoreEventIndex >= MAX_EVENTS_PER_BAR || _usedUnitsInBar >= BAR_LENGTH_UNITS) {
        finishBarMotion();
        return;
    }

    ServoScoreEvent event = SCORE_TABLE[_currentBar][_scoreEventIndex];
    _scoreEventIndex++;

    if (event.type == SCORE_END || event.lengthUnits == 0) {
        finishBarMotion();
        return;
    }

    uint8_t lengthUnits = event.lengthUnits;
    if (_usedUnitsInBar + lengthUnits > BAR_LENGTH_UNITS) {
        lengthUnits = BAR_LENGTH_UNITS - _usedUnitsInBar;
    }

    unsigned long eventDuration = noteLengthToMs(lengthUnits);

    if (event.type == SCORE_REST) {
        _scoreRestActive = true;
        stopMainMotionForRest();
    } else if (event.type == SCORE_PLAY) {
        _scoreRestActive = false;
        triggerScoreMotion(eventDuration);
    }

    _usedUnitsInBar += lengthUnits;
    _nextEventTime += eventDuration;
}

void EntertainmentRobot::triggerScoreMotion(unsigned long eventDuration) {
    unsigned long pulseTime = calculatePulseTime(eventDuration);

#if INSTRUMENT == PIANO
    if (_pianoUseRightArm) {
        triggerRightArmPulse(PIANO_RIGHT_PLAY_ANGLE, pulseTime);
    } else {
        triggerLeftArmPulse(PIANO_LEFT_PLAY_ANGLE, pulseTime);
    }
    _pianoUseRightArm = !_pianoUseRightArm;

#elif INSTRUMENT == TROMBONE
    _armDirection = !_armDirection;
    if (_armDirection) {
        _rightArmServo.write(TROMBONE_FORWARD_ANGLE);
    } else {
        _rightArmServo.write(TROMBONE_BACK_ANGLE);
    }

#elif INSTRUMENT == VIOLIN
    _armDirection = !_armDirection;
    if (_armDirection) {
        _rightArmServo.write(VIOLIN_BOW_LEFT_ANGLE);
    } else {
        _rightArmServo.write(VIOLIN_BOW_RIGHT_ANGLE);
    }

#elif INSTRUMENT == CASTANET
    triggerRightArmPulse(CASTANET_PLAY_ANGLE, pulseTime);
#endif
}

void EntertainmentRobot::finishBarMotion() {
    _barActive = false;
    _scoreRestActive = true;
    stopMainMotionForRest();
}

void EntertainmentRobot::startEmotion() {
    if (_motionMutedByWholeRest || _scoreRestActive) {
        return;
    }

#if INSTRUMENT == VIOLIN || INSTRUMENT == CASTANET
    _emotionActive = true;
    _emotionEndTime = millis() + _beatInterval * EMOTION_BEATS;
    _lastEmotionMoveTime = 0;
    _emotionDirection = false;
#endif
}

void EntertainmentRobot::updateEmotionMotion() {
    if (_motionMutedByWholeRest || _scoreRestActive) {
        resetEmotionServo();
        _emotionActive = false;
        return;
    }

    if (!_emotionActive) {
        return;
    }

    unsigned long now = millis();
    if (timeReached(_emotionEndTime)) {
        resetEmotionServo();
        _emotionActive = false;
        return;
    }

    unsigned long emotionInterval = _beatInterval / 2;
    if (emotionInterval < 100) {
        emotionInterval = 100;
    }

    if (now - _lastEmotionMoveTime >= emotionInterval) {
        _lastEmotionMoveTime = now;
        _emotionDirection = !_emotionDirection;

#if INSTRUMENT == VIOLIN
        if (_emotionDirection) {
            _extraServo.write(HEAD_LEFT_ANGLE);
        } else {
            _extraServo.write(HEAD_RIGHT_ANGLE);
        }
#elif INSTRUMENT == CASTANET
        if (_emotionDirection) {
            _extraServo.write(MOUTH_OPEN_ANGLE);
        } else {
            _extraServo.write(MOUTH_CLOSE_ANGLE);
        }
#endif
    }
}

void EntertainmentRobot::triggerRightArmPulse(int angle, unsigned long pulseTime) {
    _rightArmServo.write(angle);
    _rightArmReturnActive = true;
    _rightArmReturnTime = millis() + pulseTime;
}

void EntertainmentRobot::triggerLeftArmPulse(int angle, unsigned long pulseTime) {
    _leftArmServo.write(angle);
    _leftArmReturnActive = true;
    _leftArmReturnTime = millis() + pulseTime;
}

void EntertainmentRobot::triggerExtraPulse(int angle, unsigned long pulseTime) {
    _extraServo.write(angle);
    _extraReturnActive = true;
    _extraReturnTime = millis() + pulseTime;
}

void EntertainmentRobot::updateServoReturn() {
    unsigned long now = millis();

    if (_rightArmReturnActive && timeReached(_rightArmReturnTime)) {
        _rightArmServo.write(ARM_REST_ANGLE);
        _rightArmReturnActive = false;
    }

    if (_leftArmReturnActive && timeReached(_leftArmReturnTime)) {
        _leftArmServo.write(ARM_REST_ANGLE);
        _leftArmReturnActive = false;
    }

    if (_extraReturnActive && timeReached(_extraReturnTime)) {
        resetEmotionServo();
        _extraReturnActive = false;
    }
}

void EntertainmentRobot::stopAllMotionForWholeRest() {
    _barActive = false;
    _scoreEventIndex = 0;
    _usedUnitsInBar = 0;
    _emotionActive = false;
    cancelServoReturns();
    resetMainServos();
    resetEmotionServo();
}

void EntertainmentRobot::stopMainMotionForRest() {
    _rightArmReturnActive = false;
    _leftArmReturnActive = false;
    resetMainServos();
}

void EntertainmentRobot::cancelServoReturns() {
    _rightArmReturnActive = false;
    _leftArmReturnActive = false;
    _extraReturnActive = false;
}

void EntertainmentRobot::resetMainServos() {
#if INSTRUMENT == PIANO
    _rightArmServo.write(ARM_REST_ANGLE);
    _leftArmServo.write(ARM_REST_ANGLE);
#elif INSTRUMENT == TROMBONE
    _rightArmServo.write(ARM_REST_ANGLE);
#elif INSTRUMENT == VIOLIN
    _rightArmServo.write(ARM_REST_ANGLE);
#elif INSTRUMENT == CASTANET
    _rightArmServo.write(ARM_REST_ANGLE);
#endif
}

void EntertainmentRobot::resetEmotionServo() {
#if INSTRUMENT == VIOLIN
    _extraServo.write(HEAD_CENTER_ANGLE);
#elif INSTRUMENT == CASTANET
    _extraServo.write(MOUTH_CLOSE_ANGLE);
#endif
}

bool EntertainmentRobot::timeReached(unsigned long targetTime) {
    return (long)(millis() - targetTime) >= 0;
}