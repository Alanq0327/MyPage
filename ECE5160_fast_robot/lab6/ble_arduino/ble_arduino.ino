#include "BLECStringCharacteristic.h"
#include "EString.h"
#include "RobotCommand.h"
#include <ArduinoBLE.h>
#include <math.h>

#include <Wire.h>
#include "SparkFun_VL53L1X.h"

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 256
#endif

//////////// BLE UUIDs ////////////
#define BLE_UUID_TEST_SERVICE "a78bfeab-75da-4385-a5a6-60f3e22a62d9"
#define BLE_UUID_RX_STRING    "9750f60b-9c9c-4158-b620-02ec9521cd99"
#define BLE_UUID_TX_FLOAT     "27616294-3063-4ecc-b60b-3470ddef2938"
#define BLE_UUID_TX_STRING    "f235a225-6735-4d73-94cb-ee5dfce9ba83"
//////////// BLE UUIDs ////////////

BLEService testService(BLE_UUID_TEST_SERVICE);

BLECStringCharacteristic rx_characteristic_string(
    BLE_UUID_RX_STRING, BLEWrite, MAX_MSG_SIZE);

BLEFloatCharacteristic tx_characteristic_float(
    BLE_UUID_TX_FLOAT, BLERead | BLENotify);

BLECStringCharacteristic tx_characteristic_string(
    BLE_UUID_TX_STRING, BLERead | BLENotify, MAX_MSG_SIZE);

// ------------------ ToF ------------------
SFEVL53L1X tof;

// ------------------ RX/TX ------------------
RobotCommand robot_cmd(":|");
EString tx_estring_value;
float tx_float_value = 0.0;

// ------------------ motor pins ------------------
const int L_IN1 = 16;
const int L_IN2 = 15;
const int R_IN1 = 2;
const int R_IN2 = 14;

// ============================================================
// BLE + ToF + PID
// ToF strategy:
// 1) only one read entrance
// 2) only update on valid frame
// 3) never clear validity on single bad frame
// 4) invalid only when last valid sample is too old
// ============================================================

// ---------- ToF state ----------
bool tof_has_valid = false;
uint16_t current_tof_reading = 0;
unsigned long current_tof_time_ms = 0;
uint8_t current_tof_status = 255;

// 只要最近一次有效值不超过这个时间，就继续用
const unsigned long TOF_FRESH_TIMEOUT_MS = 180;

// ---------- PID params ----------
float Kp_pos = 0.50f;
float Ki_pos = 0.00f;
float Kd_pos = 0.15f;
int pid_pos_target = 304;   // mm

// ---------- anti-windup ----------
bool anti_windup_enable = false;   // 做 5000-level 对照时改 true / false
const float ITERM_MAX = 80.0f;    // I_term 限幅
const float INTEGRAL_MAX = 4000.0f; // 积分器本体限幅，双保险

// ---------- control state ----------
bool pid_run_active = false;
unsigned long pid_run_start_ms = 0;
unsigned long pid_run_duration_ms = 0;
unsigned long pid_last_ctrl_ms = 0;
unsigned long pid_last_log_ms = 0;

const unsigned long PID_CTRL_PERIOD_MS = 40;
unsigned long pid_log_period_ms = 150;

float current_error = 0.0f;
float prev_error_for_i = 0.0f;

float P_term = 0.0f;
float I_term = 0.0f;
float D_term = 0.0f;
float control_u = 0.0f;

float error_integral = 0.0f;

float dist_rate = 0.0f;
float dist_rate_filt = 0.0f;
int pwm_cmd = 0;
bool control_has_valid_dist = false;

// derivative state
bool d_state_ready = false;
uint16_t prev_tof_for_d = 0;
unsigned long prev_tof_for_d_time_ms = 0;

// ---------- motor config ----------
const float STOP_BAND_MM = 20.0f;
const float STOP_RATE_BAND_MMPS = 120.0f;
const int PWM_MIN = 65;
const int PWM_MAX = 140;

// ---------- log buffer ----------
const int PID_LOG_BUF_SIZE = 250;
unsigned long pid_t_ms_buf[PID_LOG_BUF_SIZE];
float pid_dist_buf[PID_LOG_BUF_SIZE];
float pid_err_buf[PID_LOG_BUF_SIZE];
float pid_p_buf[PID_LOG_BUF_SIZE];
float pid_i_buf[PID_LOG_BUF_SIZE];
float pid_d_buf[PID_LOG_BUF_SIZE];
float pid_rate_buf[PID_LOG_BUF_SIZE];
float pid_u_buf[PID_LOG_BUF_SIZE];
int pid_pwm_buf[PID_LOG_BUF_SIZE];
int pid_log_len = 0;

// ============================================================

enum CommandTypes
{
  PING = 0,
  SEND_TWO_INTS = 1,
  SEND_THREE_FLOATS = 2,
  ECHO = 3,
  DANCE = 4,
  SET_VEL = 5,
  GET_TIME_MILLIS = 6,

  START_TIME_STREAM = 7,
  STOP_TIME_STREAM = 8,
  SEND_TIME_DATA = 9,
  GET_TEMP_READINGS = 10,

  PING_LEN = 11,
  START_SEQ_STREAM = 12,
  STOP_SEQ_STREAM = 13,

  SET_PID_GAINS = 14,
  START_PID_RUN = 15,
  SEND_PID_LOG = 16,

  MOTOR_FWD = 17,
  MOTOR_BWD = 18,
  MOTOR_BRAKE = 19,
};

static void send_end_marker()
{
  BLE.poll(); delay(40);
  tx_characteristic_string.writeValue("END");
  BLE.poll(); delay(40);
  tx_characteristic_string.writeValue("END");
  BLE.poll(); delay(40);
}

// ---------- Motor helpers ----------
// 注意：按你的车当前接线，forward / backward 的物理方向本来就是反的
static void motor_forward_pwm(int pwm)
{
  analogWrite(L_IN1, pwm);
  analogWrite(L_IN2, 0);
  analogWrite(R_IN1, pwm);
  analogWrite(R_IN2, 0);
}

static void motor_backward_pwm(int pwm)
{
  analogWrite(L_IN1, 0);
  analogWrite(L_IN2, pwm);
  analogWrite(R_IN1, 0);
  analogWrite(R_IN2, pwm);
}

static void motor_active_brake()
{
  analogWrite(L_IN1, 0);
  analogWrite(L_IN2, 0);
  analogWrite(R_IN1, 0);
  analogWrite(R_IN2, 0);
}

// ---------- ToF ----------
static void update_current_tof_reading()
{
  if (!tof.checkForDataReady()) return;

  uint16_t mm = tof.getDistance();
  uint8_t status = tof.getRangeStatus();
  tof.clearInterrupt();

  current_tof_status = status;

  if (status == 0 && mm >= 40 && mm <= 4000)
  {
    current_tof_reading = mm;
    current_tof_time_ms = millis();
    tof_has_valid = true;
  }
}

static bool tof_recent_valid_available()
{
  if (!tof_has_valid) return false;
  return (millis() - current_tof_time_ms) <= TOF_FRESH_TIMEOUT_MS;
}

// ---------- PID log ----------
static void pid_append_log(unsigned long t_rel_ms)
{
  if (pid_log_len >= PID_LOG_BUF_SIZE) return;

  pid_t_ms_buf[pid_log_len]    = t_rel_ms;
  pid_dist_buf[pid_log_len]    = control_has_valid_dist ? (float)current_tof_reading : -1.0f;
  pid_err_buf[pid_log_len]     = current_error;
  pid_p_buf[pid_log_len]       = P_term;
  pid_i_buf[pid_log_len]       = I_term;
  pid_d_buf[pid_log_len]       = D_term;
  pid_rate_buf[pid_log_len]    = dist_rate_filt;
  pid_u_buf[pid_log_len]       = control_u;
  pid_pwm_buf[pid_log_len]     = pwm_cmd;
  pid_log_len++;
}

static void send_pid_log_csv()
{
  int n = pid_log_len;

  char hdr[32];
  snprintf(hdr, sizeof(hdr), "N:%d", n);
  tx_characteristic_string.writeValue(hdr);
  BLE.poll();
  delay(60);

  tx_characteristic_string.writeValue("time_ms,dist_mm,error_mm,p_term,i_term,d_term,rate_mmps,u,pwm");
  BLE.poll();
  delay(40);

  for (int i = 0; i < n; i++)
  {
    char msg[MAX_MSG_SIZE];
    snprintf(msg, sizeof(msg),
             "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d",
             pid_t_ms_buf[i],
             pid_dist_buf[i],
             pid_err_buf[i],
             pid_p_buf[i],
             pid_i_buf[i],
             pid_d_buf[i],
             pid_rate_buf[i],
             pid_u_buf[i],
             pid_pwm_buf[i]);

    tx_characteristic_string.writeValue(msg);
    BLE.poll();
    delay(12);
  }

  send_end_marker();
}

// ---------- PID control ----------
static void pid_pos_control()
{
  if (!tof_recent_valid_available())
  {
    control_has_valid_dist = false;
    current_error = 0.0f;
    P_term = 0.0f;
    I_term = 0.0f;
    D_term = 0.0f;
    control_u = 0.0f;
    dist_rate = 0.0f;
    dist_rate_filt = 0.0f;
    pwm_cmd = 0;
    motor_active_brake();
    d_state_ready = false;
    return;
  }

  control_has_valid_dist = true;

  // 正值：太远
  // 负值：太近
  current_error = (float)current_tof_reading - (float)pid_pos_target;

  // ----- D term: derivative on measurement -----
  if (!d_state_ready)
  {
    prev_tof_for_d = current_tof_reading;
    prev_tof_for_d_time_ms = current_tof_time_ms;
    dist_rate = 0.0f;
    dist_rate_filt = 0.0f;
    d_state_ready = true;
  }
  else
  {
    unsigned long dt_ms = current_tof_time_ms - prev_tof_for_d_time_ms;
    if (dt_ms > 0)
    {
      float dt = ((float)dt_ms) / 1000.0f;
      dist_rate = ((float)current_tof_reading - (float)prev_tof_for_d) / dt;
      dist_rate_filt = 0.7f * dist_rate_filt + 0.3f * dist_rate;
    }

    prev_tof_for_d = current_tof_reading;
    prev_tof_for_d_time_ms = current_tof_time_ms;
  }

  // near target and moving slowly -> stop
  if (fabs(current_error) < STOP_BAND_MM && fabs(dist_rate_filt) < STOP_RATE_BAND_MMPS)
  {
    P_term = 0.0f;
    I_term = 0.0f;
    D_term = 0.0f;
    control_u = 0.0f;
    pwm_cmd = 0;
    motor_active_brake();
    return;
  }

  // dt for integral
  float dt_i = ((float)PID_CTRL_PERIOD_MS) / 1000.0f;

  // ----- P -----
  P_term = Kp_pos * current_error;

  // ----- I -----
  error_integral += current_error * dt_i;

  // integral raw clamp
  if (error_integral > INTEGRAL_MAX) error_integral = INTEGRAL_MAX;
  if (error_integral < -INTEGRAL_MAX) error_integral = -INTEGRAL_MAX;

  I_term = Ki_pos * error_integral;

  // anti-windup clamp on I term
  if (anti_windup_enable)
  {
    if (I_term > ITERM_MAX) I_term = ITERM_MAX;
    if (I_term < -ITERM_MAX) I_term = -ITERM_MAX;

    // keep integral consistent with clamped I term
    if (fabs(Ki_pos) > 1e-6f)
    {
      error_integral = I_term / Ki_pos;
    }
  }

  // ----- D -----
  D_term = Kd_pos * dist_rate_filt;

  control_u = P_term + I_term + D_term;

  int pwm = PWM_MIN + (int)fabs(control_u);
  if (pwm > PWM_MAX) pwm = PWM_MAX;

  if (fabs(control_u) < 1e-3f)
  {
    pwm_cmd = 0;
    motor_active_brake();
    return;
  }

  // 按你的车当前方向定义
  if (control_u > 0)
  {
    pwm_cmd = pwm;
    motor_forward_pwm(pwm);
  }
  else
  {
    pwm_cmd = -pwm;
    motor_backward_pwm(pwm);
  }
}

static void pid_service()
{
  if (!pid_run_active) return;

  unsigned long now_ms = millis();

  if ((now_ms - pid_run_start_ms) >= pid_run_duration_ms)
  {
    pid_run_active = false;
    motor_active_brake();
    tx_characteristic_string.writeValue("PID:DONE");
    Serial.println("PID run done");
    return;
  }

  if ((now_ms - pid_last_ctrl_ms) >= PID_CTRL_PERIOD_MS)
  {
    pid_last_ctrl_ms = now_ms;
    pid_pos_control();
  }

  if ((now_ms - pid_last_log_ms) >= pid_log_period_ms)
  {
    pid_last_log_ms = now_ms;
    pid_append_log(now_ms - pid_run_start_ms);

    // CSV format:
    // time_ms,dist_mm,error_mm,p_term,i_term,d_term,rate_mmps,u,pwm
    if (!control_has_valid_dist) {
      Serial.print(now_ms - pid_run_start_ms);
      Serial.println(",-1,0,0,0,0,0,0,0");
    } else {
      Serial.print(now_ms - pid_run_start_ms);
      Serial.print(",");
      Serial.print(current_tof_reading);
      Serial.print(",");
      Serial.print(current_error);
      Serial.print(",");
      Serial.print(P_term);
      Serial.print(",");
      Serial.print(I_term);
      Serial.print(",");
      Serial.print(D_term);
      Serial.print(",");
      Serial.print(dist_rate_filt);
      Serial.print(",");
      Serial.print(control_u);
      Serial.print(",");
      Serial.println(pwm_cmd);
    }
  }
}

// ---------- Command ----------
void handle_command()
{
  robot_cmd.set_cmd_string(rx_characteristic_string.value(),
                           rx_characteristic_string.valueLength());

  bool success;
  int cmd_type = -1;

  success = robot_cmd.get_command_type(cmd_type);
  if (!success) return;

  switch (cmd_type) {

    case PING: {
      tx_estring_value.clear();
      tx_estring_value.append("PONG");
      tx_characteristic_string.writeValue(tx_estring_value.c_str());
      break;
    }

    case GET_TIME_MILLIS: {
      char msg[32];
      snprintf(msg, sizeof(msg), "T:%lu", millis());
      tx_characteristic_string.writeValue(msg);
      break;
    }

    case MOTOR_FWD: {
      int pwm = 0;
      success = robot_cmd.get_next_value(pwm);
      if (!success) return;
      if (pwm < 0) pwm = 0;
      if (pwm > 255) pwm = 255;
      motor_forward_pwm(pwm);
      Serial.print("MOTOR_FWD pwm = ");
      Serial.println(pwm);
      tx_characteristic_string.writeValue("MOTOR:FWD");
      break;
    }

    case MOTOR_BWD: {
      int pwm = 0;
      success = robot_cmd.get_next_value(pwm);
      if (!success) return;
      if (pwm < 0) pwm = 0;
      if (pwm > 255) pwm = 255;
      motor_backward_pwm(pwm);
      Serial.print("MOTOR_BWD pwm = ");
      Serial.println(pwm);
      tx_characteristic_string.writeValue("MOTOR:BWD");
      break;
    }

    case MOTOR_BRAKE: {
      motor_active_brake();
      Serial.println("MOTOR_BRAKE");
      tx_characteristic_string.writeValue("MOTOR:BRAKE");
      break;
    }

    // format: Kp:Ki:Kd
    case SET_PID_GAINS: {
      float kp_in, ki_in, kd_in;
      success = robot_cmd.get_next_value(kp_in); if (!success) return;
      success = robot_cmd.get_next_value(ki_in); if (!success) return;
      success = robot_cmd.get_next_value(kd_in); if (!success) return;
Kp_pos = kp_in;
      Ki_pos = ki_in;
      Kd_pos = kd_in;

      char msg[96];
      snprintf(msg, sizeof(msg), "PID:GAINS:Kp=%.3f:Ki=%.3f:Kd=%.3f", Kp_pos, Ki_pos, Kd_pos);
      tx_characteristic_string.writeValue(msg);

      Serial.print("PID gains set -> Kp=");
      Serial.print(Kp_pos);
      Serial.print(" Ki=");
      Serial.print(Ki_pos);
      Serial.print(" Kd=");
      Serial.println(Kd_pos);
      break;
    }

    // format: duration_ms:sample_ms:target_mm
    case START_PID_RUN: {
      int duration_ms = 0;
      int sample_ms = 0;
      int target_mm = 304;

      success = robot_cmd.get_next_value(duration_ms); if (!success) return;
      success = robot_cmd.get_next_value(sample_ms); if (!success) return;
      success = robot_cmd.get_next_value(target_mm); if (!success) return;

      if (duration_ms < 100) duration_ms = 100;
      if (sample_ms < 20) sample_ms = 20;
      if (target_mm < 50) target_mm = 50;

      pid_pos_target = target_mm;
      pid_run_duration_ms = (unsigned long)duration_ms;
      pid_log_period_ms = (unsigned long)sample_ms;

      current_error = 0.0f;
      prev_error_for_i = 0.0f;

      P_term = 0.0f;
      I_term = 0.0f;
      D_term = 0.0f;
      control_u = 0.0f;

      error_integral = 0.0f;

      pwm_cmd = 0;
      control_has_valid_dist = false;
      d_state_ready = false;
      dist_rate = 0.0f;
      dist_rate_filt = 0.0f;
      pid_log_len = 0;

      motor_active_brake();

      pid_run_active = true;
      pid_run_start_ms = millis();
      pid_last_ctrl_ms = pid_run_start_ms;
      pid_last_log_ms = pid_run_start_ms;

      char msg[64];
      snprintf(msg, sizeof(msg), "PID:START:T=%d", pid_pos_target);
      tx_characteristic_string.writeValue(msg);

      Serial.print("PID run started, target = ");
      Serial.print(pid_pos_target);
      Serial.print(" mm, anti_windup=");
      Serial.println(anti_windup_enable ? "ON" : "OFF");
      Serial.println("time_ms,dist_mm,error_mm,p_term,i_term,d_term,rate_mmps,u,pwm");
      break;
    }

    case SEND_PID_LOG: {
      bool was_running = pid_run_active;
      pid_run_active = false;
      send_pid_log_csv();
      pid_run_active = was_running;
      break;
    }

    default: {
      break;
    }
  }
}

// ---------- Init ----------
static void init_tof_or_die()
{
  Wire.begin();
  delay(50);

  int err = tof.begin();
  if (err != 0)
  {
    Serial.print("ToF init failed, err = ");
    Serial.println(err);
    while (1) { delay(100); }
  }

  tof.setDistanceModeShort();
  tof.setTimingBudgetInMs(50);
  tof.startRanging();

  Serial.println("ToF OK");
}

void setup()
{
  Serial.begin(115200);
  delay(1500);

  pinMode(L_IN1, OUTPUT);
  pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT);
  pinMode(R_IN2, OUTPUT);
  motor_active_brake();

  init_tof_or_die();

  if (!BLE.begin()) {
    Serial.println("BLE.begin() failed");
    while (1) { delay(100); }
  }

  BLE.setDeviceName("Artemis BLE");
  BLE.setLocalName("Artemis BLE");
  BLE.setAdvertisedService(testService);

  testService.addCharacteristic(tx_characteristic_float);
  testService.addCharacteristic(tx_characteristic_string);
  testService.addCharacteristic(rx_characteristic_string);

  BLE.addService(testService);

  tx_characteristic_float.writeValue(0.0);
  tx_characteristic_string.writeValue("READY");

  Serial.print("Advertising BLE with MAC: ");
  Serial.println(BLE.address());

  BLE.advertise();
}

// ---------- Loop ----------
void read_data()
{
  if (rx_characteristic_string.written()) {
    handle_command();
  }
}

void loop()
{
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      BLE.poll();

      // 第一优先级：后台更新最近一次有效 ToF
      update_current_tof_reading();

      // 第二优先级：控制
      if (pid_run_active) {
        pid_service();
      }

      // 最后：收命令
      read_data();
    }

    motor_active_brake();
    pid_run_active = false;

    Serial.println("Disconnected");
  }
}