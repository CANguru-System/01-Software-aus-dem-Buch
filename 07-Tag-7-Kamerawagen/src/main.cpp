/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "dl_lib_matrix3d.h"
#include "esp_http_server.h"
#include "camera_index.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Dieser Quellcode ist fast vollständig im Original übernommen
// Es wurde lediglich auf den eingesetzten Kameratyp angepasst und
// es wurden ein paar Kommentare hinzugefügt
// Sie müssen lediglich Ihre SSID (beispielsweise "FRITZ!BOX XXX") und das
// zugehörige Passwort in den nächsten Zeilen gegen die Platzhalter austauschen
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/* WiFi network name and password */
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

typedef struct
{
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;
#define LED GPIO_NUM_16
#define PART_BOUNDARY "123456789000000000000987654321"

// das ist unser Typ
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23
#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21

static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

// mit dieser und einigen folgenden Prozeduren wird der Videostream aufgenommen
// und weitergeleitet
static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        if (fb->format != PIXFORMAT_JPEG)
        {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK)
    {
      break;
    }
    //  Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

// hiermit werden alle Parameter angezeigt und ggf. auch angepasst
// Der Datenaustausch geschieht mit Hilfe einer JSON-Datei. JSON steht
// für "JavaScript Object Notation".
static esp_err_t status_handler(httpd_req_t *req)
{
  static char json_response[1024];

  sensor_t *s = esp_camera_sensor_get();
  char *p = json_response;
  *p++ = '{';

  p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p += sprintf(p, "\"quality\":%u,", s->status.quality);
  p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
  p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p += sprintf(p, "\"awb\":%u,", s->status.awb);
  p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p += sprintf(p, "\"aec\":%u,", s->status.aec);
  p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p += sprintf(p, "\"agc\":%u,", s->status.agc);
  p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
  p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
  *p++ = '}';
  *p++ = 0;
  // Die übertragenen Daten liegen im JSON-Format vor
  httpd_resp_set_type(req, "application/json");
  /*
  Dies ist ein Schutzmechanismus.
  Wenn der Browser auf dem PC Inhalte (den Kamerastream) von dem Kameraboard übernehmen will,
  sendet das Kameraboard einen Access-Control-Allow-Origin Antwort-Header. Damit zeigt das Board an,
  dass auf die Inhalte dieser Seite zugegriffen werden kann.
  */
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

// hiermit werden die Parameter für den Browser übertragen
static esp_err_t index_handler(httpd_req_t *req)
{
  // Diese Daten liegen im HTML-Format vor
  httpd_resp_set_type(req, "text/html");
  // und werden mit dem gzip komprimiert. Das ist ein Format,
  //das das Lempel-Ziv coding (LZ77) mit einem 32-bit CRC nutzt. 
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  // die Daten stehen in der Datei camera_index.h
  return httpd_resp_send(req, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
}

// Parameter können angepasst werden
static esp_err_t cmd_handler(httpd_req_t *req)
{
  char *buf;
  size_t buf_len;
  char variable[32] = { 0, };
  char value[32] = { 0, };

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    buf = (char *)malloc(buf_len);
    if (!buf)
    {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
      {
      }
      else
      {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    }
    else
    {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  }
  else
  {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int val = atoi(value);
  sensor_t *s = esp_camera_sensor_get();
  int res = 0;

  if (!strcmp(variable, "framesize"))
  {
    if (s->pixformat == PIXFORMAT_JPEG)
      res = s->set_framesize(s, (framesize_t)val);
  }
  else if (!strcmp(variable, "quality"))
    res = s->set_quality(s, val);
  else if (!strcmp(variable, "contrast"))
    res = s->set_contrast(s, val);
  else if (!strcmp(variable, "brightness"))
    res = s->set_brightness(s, val);
  else if (!strcmp(variable, "saturation"))
    res = s->set_saturation(s, val);
  else if (!strcmp(variable, "gainceiling"))
    res = s->set_gainceiling(s, (gainceiling_t)val);
  else if (!strcmp(variable, "colorbar"))
    res = s->set_colorbar(s, val);
  else if (!strcmp(variable, "awb"))
    res = s->set_whitebal(s, val);
  else if (!strcmp(variable, "agc"))
    res = s->set_gain_ctrl(s, val);
  else if (!strcmp(variable, "aec"))
    res = s->set_exposure_ctrl(s, val);
  else if (!strcmp(variable, "hmirror"))
    res = s->set_hmirror(s, val);
  else if (!strcmp(variable, "vflip"))
    res = s->set_vflip(s, val);
  else if (!strcmp(variable, "awb_gain"))
    res = s->set_awb_gain(s, val);
  else if (!strcmp(variable, "agc_gain"))
    res = s->set_agc_gain(s, val);
  else if (!strcmp(variable, "aec_value"))
    res = s->set_aec_value(s, val);
  else if (!strcmp(variable, "aec2"))
    res = s->set_aec2(s, val);
  else if (!strcmp(variable, "dcw"))
    res = s->set_dcw(s, val);
  else if (!strcmp(variable, "bpc"))
    res = s->set_bpc(s, val);
  else if (!strcmp(variable, "wpc"))
    res = s->set_wpc(s, val);
  else if (!strcmp(variable, "raw_gma"))
    res = s->set_raw_gma(s, val);
  else if (!strcmp(variable, "lenc"))
    res = s->set_lenc(s, val);
  else if (!strcmp(variable, "special_effect"))
    res = s->set_special_effect(s, val);
  else if (!strcmp(variable, "wb_mode"))
    res = s->set_wb_mode(s, val);
  else if (!strcmp(variable, "ae_level"))
    res = s->set_ae_level(s, val);
  else
  {
    res = -1;
  }

  if (res)
  {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

// die Videodaten werden per http auf den PC übertragen. Hiermit
// wird der zugehörige Server gestartet
void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL};

  httpd_uri_t status_uri = {
      .uri = "/status",
      .method = HTTP_GET,
      .handler = status_handler,
      .user_ctx = NULL};

  httpd_uri_t cmd_uri = {
      .uri = "/control",
      .method = HTTP_GET,
      .handler = cmd_handler,
      .user_ctx = NULL};

  httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  Serial.printf("Starting web server on port: '%d'", config.server_port);
  Serial.println();
  if (httpd_start(&camera_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  Serial.printf("Starting stream server on port: '%d'", config.server_port);
  Serial.println();
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  // initialize LED digital pin as an output.
  pinMode(LED, OUTPUT);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED, HIGH);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Wi-Fi connection
  bool onoff = true;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (onoff)
    {
      // turn the LED on by making the voltage HIGH
      digitalWrite(LED, HIGH);
    }
    else
    {
      // turn the LED on by making the voltage HIGH
      digitalWrite(LED, LOW);
    }
    onoff = !onoff;
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // turn the LED on by making the voltage HIGH
  digitalWrite(LED, HIGH);

  // Start streaming web server
  startCameraServer();
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
}

void loop()
{
  delay(1);
}
