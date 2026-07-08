#ifndef ENTERTAINMENT_ROBOT_H
#define ENTERTAINMENT_ROBOT_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <Servo.h>

// =====================
// 楽器選択定義
// =====================
#define PIANO      1
#define TROMBONE   2
#define VIOLIN     3
#define CASTANET   4

// 使う人形に合わせてここを変更
//#define INSTRUMENT PIANO
//#define INSTRUMENT TROMBONE
//#define INSTRUMENT VIOLIN
#define INSTRUMENT CASTANET

// =====================
// 楽譜データ駆動のための構造体・列挙型
// =====================
enum ScoreEventType {
  SCORE_END  = 0,
  SCORE_PLAY = 1,
  SCORE_REST = 2
};

enum NoteLengthUnit {
  LENGTH_SIXTEENTH = 1,
  LENGTH_EIGHTH    = 2,
  LENGTH_QUARTER   = 4,
  LENGTH_HALF      = 8,
  LENGTH_WHOLE     = 16
};

struct ServoScoreEvent {
  ScoreEventType type;
  uint8_t lengthUnits;
};

class EntertainmentRobot {
public:
    EntertainmentRobot();
    void begin(const char* ssid, const char* pass, uint16_t port);
    void update();

private:
    // 通信関連
    WiFiUDP _udp;
    uint16_t _localPort;

    // サーボオブジェクト
    Servo _rightArmServo;
    Servo _leftArmServo;
    Servo _extraServo;

    // ピン配置定数
    static const int RIGHT_ARM_PIN = 9;
    static const int LEFT_ARM_PIN  = 11; // 新コードに合わせて10番に修正
    static const int EXTRA_PIN      = 5;

    // サーボ角度定数
    static const int ARM_REST_ANGLE = 90;
    static const int PIANO_RIGHT_PLAY_ANGLE = 70;
    static const int PIANO_LEFT_PLAY_ANGLE  = 110;

    static const int TROMBONE_FORWARD_ANGLE = 65;
    static const int TROMBONE_BACK_ANGLE    = 115;

    static const int VIOLIN_BOW_LEFT_ANGLE  = 65;
    static const int VIOLIN_BOW_RIGHT_ANGLE = 115;

    static const int CASTANET_PLAY_ANGLE    = 70;

    static const int HEAD_CENTER_ANGLE      = 90;
    static const int HEAD_LEFT_ANGLE        = 90; // 70度に修正
    static const int HEAD_RIGHT_ANGLE       = 180; // 110度に修正

    static const int MOUTH_CLOSE_ANGLE      = 90; // 90度に修正
    static const int MOUTH_OPEN_ANGLE       = 180; // 60度に修正

    // 全休符・オフセット設定
    static const uint8_t SERVER_BAR_COUNT = 25;
    static const int PART_BAR_OFFSET = 0; // 必要に応じて変更してください
    static const bool WHOLE_REST_BAR_TABLE[SERVER_BAR_COUNT];

    // 楽譜テーブルの定数
    static const uint8_t MAX_EVENTS_PER_BAR = 8;
    static const uint8_t BAR_LENGTH_UNITS = 16;
    static const uint8_t QUARTER_LENGTH_UNITS = 4;
    static const ServoScoreEvent SCORE_TABLE[SERVER_BAR_COUNT][MAX_EVENTS_PER_BAR];

    // 状態管理変数
    uint8_t _currentBPM;
    uint8_t _currentBar;
    unsigned long _beatInterval;
    
    // 楽譜イベント駆動用のタイマー変数
    unsigned long _nextEventTime;
    uint8_t _scoreEventIndex;
    uint8_t _usedUnitsInBar;

    bool _barActive;
    bool _armDirection;
    bool _motionMutedByWholeRest; // 全休符ミュートフラグ
    bool _scoreRestActive;        // 小節内の部分休符フラグ
    bool _pianoUseRightArm;       // ピアノの左右腕交互フラグ

    // サーボパルス（タイマー）管理
    bool _rightArmReturnActive;
    bool _leftArmReturnActive;
    bool _extraReturnActive;
    unsigned long _rightArmReturnTime;
    unsigned long _leftArmReturnTime;
    unsigned long _extraReturnTime;

    // BPM変更エフェクト管理
    bool _emotionActive;
    unsigned long _emotionEndTime;
    unsigned long _lastEmotionMoveTime;
    bool _emotionDirection;
    static const int EMOTION_BEATS = 4;

    // 内部メソッド
    void setupServos();
    void receiveUdpData();
    void updateBPM(uint8_t newBPM);
    void calculateBeatInterval();
    unsigned long noteLengthToMs(uint8_t lengthUnits);
    unsigned long calculatePulseTime(unsigned long eventDuration);
    uint8_t toScoreBar(uint8_t serverBarNumber);
    bool isWholeRestBar(uint8_t scoreBarNumber);
    void startBarMotion(uint8_t barNumber);
    void updateNormalMotion();
    void triggerNextScoreEvent();
    void triggerScoreMotion(unsigned long eventDuration);
    void finishBarMotion();
    void startEmotion();
    void updateEmotionMotion();
    
    void triggerRightArmPulse(int angle, unsigned long pulseTime);
    void triggerLeftArmPulse(int angle, unsigned long pulseTime);
    void triggerExtraPulse(int angle, unsigned long pulseTime);
    void updateServoReturn();
    void stopAllMotionForWholeRest();
    void stopMainMotionForRest();
    void cancelServoReturns();
    void resetMainServos();
    void resetEmotionServo();
    bool timeReached(unsigned long targetTime);
};

#endif 