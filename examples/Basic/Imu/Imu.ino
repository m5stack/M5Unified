// Include this to enable the M5 global instance.
// #include <M5UnitOLED.h>
// #include <M5UnitLCD.h>

#include <M5Unified.h>


// キャリブレーション動作の強さ。1が弱く、255が最も強い。
// 0を指定するとキャリブレーションが行われない。
static constexpr const uint8_t calib_value = 64;


// このサンプルでは、ボタンや画面をクリックすることでキャリブレーションが行われる。
// 10秒間経過後にキャリブレーション結果がNVSに保存される。
// 保存されたキャリブレーション値は、次回起動時に読み込まれる。
// 
// キャリブレーション時の本体の動かし方
// 姿勢変化が大きい時の値は無視されるため、
// キャリブレーション時はなるべく静かに動かすことが望ましい。
// 
// ※ 加速度のキャリブレーション方法
//    本体６面を順に下にする。各面で２～３秒程度静止させるとよい。
// 
// ※ ジャイロのキャリブレーション方法
//    本体を静止させる。
// 
// ※ 地磁気のキャリブレーション方法
//    本体をゆっくり多方向に回転させる。


struct rect_t
{
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

static constexpr const uint32_t color_tbl[18] = 
{
0xFF0000u, 0xCCCC00u, 0xCC00FFu,
0xFFCC00u, 0x00FF00u, 0x0088FFu,
0xFF00CCu, 0x00FFCCu, 0x0000FFu,
0xFF0000u, 0xCCCC00u, 0xCC00FFu,
0xFFCC00u, 0x00FF00u, 0x0088FFu,
0xFF00CCu, 0x00FFCCu, 0x0000FFu,
};
static constexpr const float coefficient_tbl[3] = { 0.5f, (1.0f / 256.0f), (1.0f / 1024.0f) };

static auto &dsp = (M5.Display);
static rect_t rect_graph_area;
static rect_t rect_text_area;

// キャリブレーション動作の残り秒数
static uint8_t calib_countdown = 0;

static int prev_xpos[18];

void drawBar(int32_t ox, int32_t oy, int32_t nx, int32_t px, int32_t h, uint32_t color)
{
  uint32_t bgcolor = (color >> 3) & 0x1F1F1Fu;
  if (px && ((nx < 0) != (px < 0)))
  {
    dsp.fillRect(ox, oy, px, h, bgcolor);
    px = 0;
  }
  if (px != nx)
  {
    if ((nx > px) != (nx < 0))
    {
      bgcolor = color;
    }
    dsp.setColor(bgcolor);
    dsp.fillRect(nx + ox, oy, px - nx, h);
  }
}

void drawGraph(const rect_t& r, const m5::imu_data_t& data)
{
  float aw = (128 * r.w) >> 1;
  float gw = (128 * r.w) / 256.0f;
  float mw = (128 * r.w) / 1024.0f;
  int h = (r.h / 18) * 2;
  int ox = (r.x + r.w)>>1;
  int oy = r.y;

  float xval[18];
  for (int i = 0; i < 3; ++i)
  {
    auto coe = coefficient_tbl[i] * (128 * r.w);
    for (int j = 0; j < 3; ++j)
    {
      xval[i * 3 + j] = data.sensor[i].value[j] * coe;
    }
  }

  int bar_count = 9;

  if (calib_countdown)
  {
    bar_count = 18;
    h >>= 1;
    for (int i = 0; i < 9; ++i)
    {
      xval[9 + i] = M5.Imu.getOffsetData(i) >> 12;
    }
  }

  dsp.startWrite();
  for (int i = 0; i < bar_count; ++i)
  {
    float tmp = xval[i];
    tmp = sqrtf(fabsf(tmp)) * (signbit(tmp) ? -1 : 1);
    int nx = tmp;
    int px = prev_xpos[i];
    if (nx == px) continue;
    prev_xpos[i] = nx;
    drawBar(ox, oy + h * i, nx, px, h - 1, color_tbl[i]);
  }
  dsp.endWrite();
}

void updateCalibration(uint32_t c, bool clear = false)
{
  calib_countdown = c;

  if (c == 0) {
    clear = true;
  }

  if (clear)
  {
    memset(prev_xpos, 0, sizeof(prev_xpos));
    dsp.fillScreen(TFT_BLACK);

    if (c == 0)
    { // 加速度とジャイロのキャリブレーションを停止する。
      M5.Imu.setCalibration(0, 0, calib_value);
      // 地磁気に関しては外乱による変動が大きいため、
      // 好みに応じて常に動作させておいても良いし、停止させてもよい。
    } else {
    // 加速度・ジャイロ・地磁気のキャリブレーションの強さを指定する。
      M5.Imu.setCalibration(calib_value, calib_value, calib_value);
    // ※ これによりM5.Imu.update時に都度キャリブレーションが行われる。
    // 例えばジャイロだけキャリブレーションしたい場合は以下のようにする。
    // M5.Imu.setCalibration(0, 100, 0);
    // 引数の数値が高いほどオフセット値の変化量が大きくなる。
    }
  }

  auto backcolor = (c == 0) ? TFT_BLACK : TFT_BLUE;
  dsp.fillRect(rect_text_area.x, rect_text_area.y, rect_text_area.w, rect_text_area.h, backcolor);

  if (c)
  {
    dsp.setCursor(rect_text_area.x + 2, rect_text_area.y + 1);
    dsp.setTextColor(TFT_WHITE, TFT_BLUE);
    dsp.printf("Countdown:%d ", c);
  }
}

void startCalibration(void)
{
  updateCalibration(10, true);
}

void setup(void)
{
  auto cfg = M5.config();

  // If you want to use external IMU, write this
  // cfg.external_imu = true;

  M5.begin(cfg);

  const char* name;
  switch (M5.Imu.getType())
  {
  case m5::imu_none:        name = "not found";   break;
  case m5::imu_sh200q:      name = "sh200q";      break;
  case m5::imu_mpu6050:     name = "mpu6050";     break;
  case m5::imu_mpu6886:     name = "mpu6886";     break;
  case m5::imu_mpu9250:     name = "mpu9250";     break;
  case m5::imu_bmi270:      name = "bmi270";      break;
  default:                  name = "unknown";     break;
  };
  M5_LOGI("imu:%s", name);

  int32_t w = dsp.width();
  int32_t h = dsp.height();
  if (w < h)
  { // 横長配置に変更
    dsp.setRotation(dsp.getRotation() ^ 1);
    w = dsp.width();
    h = dsp.height();
  }
  int32_t graph_area_h = ((h - 8) / 18) * 18;
  int32_t text_area_h = h - graph_area_h;
  float fontsize = text_area_h / 8;
  dsp.setTextSize(fontsize);

  rect_graph_area = { 0, 0, w, graph_area_h };
  rect_text_area = {0, graph_area_h, w, text_area_h };

  // キャリブレーション値をNVSから読み込んで反映する
  if (!M5.Imu.loadOffsetFromNVS())
  {
    startCalibration();
  }
}

void loop(void)
{
  static uint32_t frame_count = 0;
  static uint32_t prev_sec = 0;

  // IMUの現在値を取得するには M5.Imu.updateを使用する。
  auto imu_update = M5.Imu.update();
  if (imu_update)
  { // 新しい値が取得できていれば
    m5::imu_data_t data = M5.Imu.getImuData();
    drawGraph(rect_graph_area, data);

    ++frame_count;
  }
  else
  {
    vTaskDelay(1);
  }

  M5.update();
  if (M5.BtnA.wasClicked() || M5.BtnPWR.wasClicked() || M5.Touch.getDetail().wasClicked())
  { // ボタンや画面がクリックされたときキャリブレーションを開始する。
    startCalibration();
  }

  // 毎秒の処理
  int32_t sec = millis() / 1000;
  if (prev_sec != sec)
  {
    prev_sec = sec;
    if (calib_countdown)
    {
      updateCalibration(calib_countdown - 1);
    }

    M5_LOGI("sec:%d  frame:%d", sec, frame_count);
    frame_count = 0;
    vTaskDelay(1);
  }
}
