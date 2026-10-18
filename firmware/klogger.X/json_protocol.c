#include "json_protocol.h"
#include "jsmn.h"
#include "usb_comm.h"   // USB_Comm_SendString
#include "version.h"    // FW_VERSION / PROTOCOL_VERSION
#include "main.h"           // getBatteryVoltage_mV / Clock_Set/Get / setLoggingActive
#include "eeprom_manager.h" // EM_settings / EM_saveSettings
#include "measurement.h"    // rec_latest / Measurement_ClearData
#include "w25q128.h"        // レコード読み出し
#include "tof.h"            // 距離センサ

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================
// 設定
// ============================================================
#define JP_MAX_CMD     256   // 1コマンドの最大文字数 (ラインバッファ)
#define JP_MAX_TOKENS  32    // jsmn トークン上限 (現状のコマンドには十分)
#define JP_REPLY_CAP   128   // 1応答の最大文字数

// ============================================================
// 受信ラインバッファ
// ============================================================
static char     s_line[JP_MAX_CMD];
static uint16_t s_pos = 0;

// ============================================================
// 応答送信
// ============================================================
static void send_line(const char *s)
{
    USB_Comm_SendString(s);
}

// 成功(追加フィールド無し): {"id":N,"ok":true}\n
static void reply_ok(int32_t id)
{
    char b[48];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":true}\n", (long)id);
    send_line(b);
}

// 失敗: {"id":N,"ok":false,"error":"<code>"}\n
static void reply_error(int32_t id, const char *code)
{
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":false,\"error\":\"%s\"}\n",
             (long)id, code);
    send_line(b);
}

// ============================================================
// jsmn トークン操作ヘルパ
// ============================================================

// 文字列トークン t が C 文字列 s と一致するか
static bool tok_eq(const char *json, const jsmntok_t *t, const char *s)
{
    int len = t->end - t->start;
    return t->type == JSMN_STRING
        && (int)strlen(s) == len
        && strncmp(json + t->start, s, (size_t)len) == 0;
}

// プリミティブ/文字列トークンを整数として解釈する
static int32_t tok_int(const char *json, const jsmntok_t *t)
{
    char tmp[12];
    int len = t->end - t->start;
    if (len <= 0 || len >= (int)sizeof(tmp)) return 0;
    memcpy(tmp, json + t->start, (size_t)len);
    tmp[len] = '\0';
    return (int32_t)atol(tmp);
}

// オブジェクト obj_idx の直下から key に対応する「値トークン」の index を返す。
// 見つからなければ -1。ネストした値 (object/array) も正しくスキップする。
static int obj_get(const char *json, const jsmntok_t *t, int ntok,
                   int obj_idx, const char *key)
{
    if (obj_idx < 0 || t[obj_idx].type != JSMN_OBJECT) return -1;

    int end = t[obj_idx].end;
    int i = obj_idx + 1;
    while (i < ntok && t[i].start < end)
    {
        // t[i] はキー、t[i+1] はその値
        if (tok_eq(json, &t[i], key)) return i + 1;

        // キーをスキップ → 値とその子孫をスキップ
        i++;                       // 値トークンへ
        if (i >= ntok) break;
        int vend = t[i].end;
        i++;                       // 値の最初の子へ
        while (i < ntok && t[i].start < vend) i++;
    }
    return -1;
}

// ============================================================
// コマンドハンドラ
//   id      : リクエスト id (応答に必要)
//   json    : 受信した完全な JSON 文字列
//   t       : jsmn パース結果
//   ntok    : 有効トークン数
//   params  : "params" 値トークンの index (無ければ -1)
// ============================================================
typedef void (*jp_handler_fn)(int32_t id, const char *json,
                              const jsmntok_t *t, int ntok, int params);

// 疎通確認: 追加フィールド無しで ok を返すだけ
static void ph_ping(int32_t id, const char *json,
                    const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    reply_ok(id);
}

// 機体固有IDを返す (読み取り専用, AVRシリコンシリアル由来の8桁hex)
static void ph_get_id(int32_t id, const char *json,
                      const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b),
             "{\"id\":%ld,\"ok\":true,\"device_id\":\"%08lX\"}\n",
             (long)id, (unsigned long)Device_GetID());
    send_line(b);
}

// 機器名・ファームウェア情報を返す
static void ph_hello(int32_t id, const char *json,
                     const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b),
             "{\"id\":%ld,\"ok\":true,\"device\":\"%s\",\"fw\":\"%s\",\"proto\":%d,\"build\":\"%s %s\"}\n",
             (long)id, DEVICE_NAME, FW_VERSION, (int)PROTOCOL_VERSION, FW_BUILD_DATE, FW_BUILD_TIME);
    send_line(b);
}

// 電池電圧 [mV] を返す (USB給電中で電池が無いときは0付近)
static void ph_get_battery(int32_t id, const char *json,
                           const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":true,\"vbat_mV\":%u}\n",
             (long)id, (unsigned)getBatteryVoltage_mV());
    send_line(b);
}

// params.n をそのまま返すエコー (params 抽出の疎通確認用)
static void ph_echo(int32_t id, const char *json,
                    const jsmntok_t *t, int ntok, int params)
{
    int32_t n = 0;
    if (params >= 0)
    {
        int nt = obj_get(json, t, ntok, params, "n");
        if (nt >= 0) n = tok_int(json, &t[nt]);
    }
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":true,\"n\":%ld}\n", (long)id, (long)n);
    send_line(b);
}

// 記録開始: 既存データを消去してから記録モードへ移行する。
// 消去は使用量に応じて最大数十秒かかる (16MB満杯で約40秒)。PC側は長めのタイムアウトで待つこと。
// 応答後はUSBを停止する (以降は応答しない。通信に戻すにはUSBを挿し直すかボタン5秒長押し)。
static void ph_start_logging(int32_t id, const char *json,
                             const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    if (!setLoggingActive(true)) { reply_error(id, "sensor_init_failed"); return; }
    reply_ok(id);
    USB_Comm_Flush();
    // USB有効のままだと OSCHF(12MHz) が止まらず STANDBY でも ~2mA 消費するため完全停止する
    USB_Comm_Disable();
}

// 記録停止 (USB通信モードのまま)
static void ph_stop_logging(int32_t id, const char *json,
                            const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    setLoggingActive(false);
    reply_ok(id);
}

// 日時設定: params.epoch (UNIX秒)
static void ph_set_time(int32_t id, const char *json,
                        const jsmntok_t *t, int ntok, int params)
{
    int et = (params >= 0) ? obj_get(json, t, ntok, params, "epoch") : -1;
    if (et < 0) { reply_error(id, "missing_epoch"); return; }
    Clock_Set((uint32_t)tok_int(json, &t[et]));
    reply_ok(id);
}

// 記録間隔設定: params.sec (1〜600秒)。上限はVL53L1Xの測定周期レジスタ計算(32bit)の余裕から決めている
static void ph_set_interval(int32_t id, const char *json,
                            const jsmntok_t *t, int ntok, int params)
{
    int st = (params >= 0) ? obj_get(json, t, ntok, params, "sec") : -1;
    if (st < 0) { reply_error(id, "missing_sec"); return; }
    int32_t sec = tok_int(json, &t[st]);
    if (sec < 1 || sec > 600) { reply_error(id, "invalid_sec"); return; }
    EM_settings.interval_sec = (uint16_t)sec;
    EM_saveSettings();
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":true,\"interval\":%ld}\n", (long)id, (long)sec);
    send_line(b);
}

// 測距条件の応答を作る
static void reply_tof(int32_t id)
{
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b),
        "{\"id\":%ld,\"ok\":true,\"mode\":\"%s\",\"tb_ms\":%u,\"roi\":%u,\"offset_mm\":%d}\n",
        (long)id, EM_settings.tof_mode == TOF_MODE_LONG ? "long" : "short",
        (unsigned)EM_settings.tof_tb_ms, (unsigned)EM_settings.tof_roi,
        (int)EM_settings.tof_offset_mm);
    send_line(b);
}

// 測距条件の取得
static void ph_get_tof(int32_t id, const char *json,
                       const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    reply_tof(id);
}

// 測距条件の設定: params { "mode":"short"|"long", "tb_ms":15|20|33|50|100|200|500, "roi":4..16 }
// 指定した項目だけ変更する。Longモードは tb_ms>=20。
static void ph_set_tof(int32_t id, const char *json,
                       const jsmntok_t *t, int ntok, int params)
{
    if (params < 0) { reply_error(id, "missing_params"); return; }
    uint8_t  mode = EM_settings.tof_mode;
    uint16_t tb   = EM_settings.tof_tb_ms;
    uint8_t  roi  = EM_settings.tof_roi;

    int mt = obj_get(json, t, ntok, params, "mode");
    if (mt >= 0)
    {
        if      (tok_eq(json, &t[mt], "short")) mode = TOF_MODE_SHORT;
        else if (tok_eq(json, &t[mt], "long"))  mode = TOF_MODE_LONG;
        else { reply_error(id, "invalid_mode"); return; }
    }
    int tt = obj_get(json, t, ntok, params, "tb_ms");
    if (tt >= 0) tb = (uint16_t)tok_int(json, &t[tt]);
    int rt = obj_get(json, t, ntok, params, "roi");
    if (rt >= 0) roi = (uint8_t)tok_int(json, &t[rt]);

    bool tb_ok = (tb == 15 || tb == 20 || tb == 33 || tb == 50 || tb == 100 || tb == 200 || tb == 500);
    if (!tb_ok || (mode == TOF_MODE_LONG && tb == 15)) { reply_error(id, "invalid_tb_ms"); return; }
    if (roi < 4 || roi > 16) { reply_error(id, "invalid_roi"); return; }

    EM_settings.tof_mode  = mode;
    EM_settings.tof_tb_ms = tb;
    EM_settings.tof_roi   = roi;
    EM_saveSettings();
    if (Tof_IsPowered() && !Tof_ApplySettings()) { reply_error(id, "sensor_error"); return; }
    reply_tof(id);
}

// オフセット校正: params.mm の位置(10〜1000mm)に対象物を置いて実行。50回平均。数秒〜30秒ブロックする。
static void ph_calibrate(int32_t id, const char *json,
                         const jsmntok_t *t, int ntok, int params)
{
    int mt = (params >= 0) ? obj_get(json, t, ntok, params, "mm") : -1;
    if (mt < 0) { reply_error(id, "missing_mm"); return; }
    int32_t mm = tok_int(json, &t[mt]);
    if (mm < 10 || mm > 1000) { reply_error(id, "invalid_mm"); return; }

    int16_t offset = 0;
    if (!Tof_CalibrateOffset((uint16_t)mm, 50, &offset)) { reply_error(id, "calibration_failed"); return; }
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"ok\":true,\"offset_mm\":%d}\n", (long)id, (int)offset);
    send_line(b);
}

// メモリ初期化: 保存済みデータを消去する (使用量に応じて最大数十秒)
static void ph_clear_memory(int32_t id, const char *json,
                            const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    Measurement_ClearData();
    reply_ok(id);
}

// 状態取得
static void ph_get_status(int32_t id, const char *json,
                          const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    char b[192];
    snprintf(b, sizeof(b),
        "{\"id\":%ld,\"ok\":true,\"logging\":%s,\"interval\":%u,"
        "\"count\":%lu,\"capacity\":%lu,\"epoch\":%lu,\"start_dt\":%lu}\n",
        (long)id,
        getLoggingActive() ? "true" : "false",
        (unsigned)EM_settings.interval_sec,
        (unsigned long)rec_latest,
        (unsigned long)MAX_RECORD_COUNT,
        (unsigned long)Clock_Get(),
        (unsigned long)EM_settings.start_dt);
    send_line(b);
}

// 1回測定 (保存しない)。センサを(必要なら)起動して測る。応答: distance_mm/status/signal/ambient
static void ph_measure(int32_t id, const char *json,
                       const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    TofResult_t r;
    if (!Tof_MeasureOnce(&r, 1000)) { reply_error(id, "measure_failed"); return; }
    char b[JP_REPLY_CAP];
    snprintf(b, sizeof(b),
        "{\"id\":%ld,\"ok\":true,\"distance_mm\":%u,\"status\":%u,\"signal\":%u,\"ambient\":%u}\n",
        (long)id, (unsigned)r.distance_mm, (unsigned)r.status,
        (unsigned)r.signal, (unsigned)r.ambient);
    send_line(b);
}

// セルフテスト: フラッシュ(JEDEC ID + write/read/verify) / 距離センサ(ID) / 1回測定
static void ph_selftest(int32_t id, const char *json,
                        const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;

    uint8_t fid[3] = { 0, 0, 0 };
    bool flash_id = W25_ReadJEDECID(fid);
    bool flash_rw = W25_SelfTest();

    uint16_t sid = 0;
    bool tof_on = Tof_PowerOn();
    bool tof_id = tof_on && Tof_GetSensorId(&sid) && sid == 0xEACC;

    TofResult_t r = { 0, 0, 0, 0 };
    bool meas_ok = tof_on && Tof_MeasureOnce(&r, 1000);
    bool all_ok = flash_id && flash_rw && tof_id && meas_ok;

    char b[256];
    snprintf(b, sizeof(b),
        "{\"id\":%ld,\"ok\":%s,"
        "\"flash\":{\"ok\":%s,\"rw\":%s,\"jedec\":\"%02X%02X%02X\"},"
        "\"tof\":{\"ok\":%s,\"sensor_id\":\"%04X\"},"
        "\"measure\":{\"ok\":%s,\"distance_mm\":%u,\"status\":%u}}\n",
        (long)id, all_ok ? "true" : "false",
        flash_id ? "true" : "false", flash_rw ? "true" : "false",
        fid[0], fid[1], fid[2],
        tof_id ? "true" : "false", (unsigned)sid,
        meas_ok ? "true" : "false", (unsigned)r.distance_mm, (unsigned)r.status);
    send_line(b);
}

// 1回の送出にまとめるレコード数 (2B*200=400B。CDC送信バッファ512B以内)
#define DUMP_BATCH_RECS  200

// レコード [start, start+count) を生バイナリ(uint16 little-endian)でバッチ送出する
static void dump_records_binary(uint32_t start, uint32_t count)
{
    uint16_t buf[DUMP_BATCH_RECS];
    while (count > 0)
    {
        uint16_t n = (count > DUMP_BATCH_RECS) ? DUMP_BATCH_RECS : (uint16_t)count;
        if (!W25_ReadRecords(start, buf, n)) break;
        USB_Comm_SendBytes((const uint8_t *)buf, (uint16_t)(n * RECORD_SIZE));
        start += n;
        count -= n;
    }
}

// dump ヘッダ
static void send_dump_header(int32_t id, uint32_t start, uint32_t count, uint32_t total)
{
    char b[192];
    snprintf(b, sizeof(b),
        "{\"id\":%ld,\"ok\":true,\"dump\":\"begin\",\"start\":%lu,\"count\":%lu,\"total\":%lu,"
        "\"rec_size\":%u,\"format\":\"dist13_st3\",\"start_dt\":%lu,\"interval\":%u}\n",
        (long)id, (unsigned long)start, (unsigned long)count, (unsigned long)total,
        (unsigned)RECORD_SIZE, (unsigned long)EM_settings.start_dt,
        (unsigned)EM_settings.interval_sec);
    send_line(b);
}

static void send_dump_footer(int32_t id)
{
    char b[48];
    snprintf(b, sizeof(b), "{\"id\":%ld,\"dump\":\"end\"}\n", (long)id);
    send_line(b);
    USB_Comm_Flush();
}

// データ読み出し (dump): フレーミングはJSON行、レコード本体は生バイナリ。
//   ヘッダ: {"id":N,"ok":true,"dump":"begin","start":0,"count":M,"total":M,"rec_size":2,
//            "format":"dist13_st3","start_dt":S,"interval":IV}
//   本体  : M個の uint16 (little-endian)。bit15-13=状態コード, bit12-0=距離[mm]、0xFFFE=計測失敗。
//           時刻は start_dt + i*interval。
//   フッタ: {"id":N,"dump":"end"}
static void ph_dump(int32_t id, const char *json,
                    const jsmntok_t *t, int ntok, int params)
{
    (void)json; (void)t; (void)ntok; (void)params;
    uint32_t total = rec_latest;
    send_dump_header(id, 0, total, total);
    dump_records_binary(0, total);
    send_dump_footer(id);
}

// 範囲指定データ読み出し (dump_range): params { "start": 先頭index, "count": 件数 }
// 大量データを分割して読み出す用。形式は dump と同じ (count は範囲クランプ後の件数)。
static void ph_dump_range(int32_t id, const char *json,
                          const jsmntok_t *t, int ntok, int params)
{
    int32_t start = 0, want = 0;
    if (params >= 0)
    {
        int st = obj_get(json, t, ntok, params, "start");
        int ct = obj_get(json, t, ntok, params, "count");
        if (st >= 0) start = tok_int(json, &t[st]);
        if (ct >= 0) want  = tok_int(json, &t[ct]);
    }
    if (start < 0) start = 0;
    if (want  < 0) want  = 0;

    uint32_t total = rec_latest;
    uint32_t s = (uint32_t)start;
    uint32_t c = (s >= total) ? 0 : (((uint32_t)want > total - s) ? total - s : (uint32_t)want);

    send_dump_header(id, s, c, total);
    dump_records_binary(s, c);
    send_dump_footer(id);
}

// コマンドテーブル (NULL 終端)
typedef struct {
    const char    *name;
    jp_handler_fn  handler;
} jp_command_t;

static const jp_command_t s_commands[] = {
    { "ping",          ph_ping          },
    { "hello",         ph_hello         },
    { "get_id",        ph_get_id        },
    { "get_battery",   ph_get_battery   },
    { "echo",          ph_echo          },
    { "start_logging", ph_start_logging },
    { "stop_logging",  ph_stop_logging  },
    { "set_time",      ph_set_time      },
    { "set_interval",  ph_set_interval  },
    { "get_tof",       ph_get_tof       },
    { "set_tof",       ph_set_tof       },
    { "calibrate",     ph_calibrate     },
    { "clear_memory",  ph_clear_memory  },
    { "get_status",    ph_get_status    },
    { "measure",       ph_measure       },
    { "selftest",      ph_selftest      },
    { "dump",          ph_dump          },
    { "dump_range",    ph_dump_range    },
    { NULL,            NULL             }
};

// ============================================================
// ディスパッチ
// ============================================================
void JP_ProcessLine(const char *line)
{
    // JSON コマンドのみ受理 ('{' 始まり以外は無視)
    if (line[0] != '{') return;

    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t toks[JP_MAX_TOKENS];
    int n = jsmn_parse(&parser, line, strlen(line), toks, JP_MAX_TOKENS);

    if (n < 1 || toks[0].type != JSMN_OBJECT)
    {
        reply_error(0, "parse_error");
        return;
    }

    // id を抽出 (無ければ 0)
    int32_t id = 0;
    int id_tok = obj_get(line, toks, n, 0, "id");
    if (id_tok >= 0) id = tok_int(line, &toks[id_tok]);

    // command を抽出
    int cmd_tok = obj_get(line, toks, n, 0, "command");
    if (cmd_tok < 0 || toks[cmd_tok].type != JSMN_STRING)
    {
        reply_error(id, "missing_command");
        return;
    }

    // params (任意)。あればオブジェクトであること。
    int params_tok = obj_get(line, toks, n, 0, "params");
    if (params_tok >= 0 && toks[params_tok].type != JSMN_OBJECT)
    {
        reply_error(id, "invalid_params");
        return;
    }

    // テーブル検索してディスパッチ
    for (const jp_command_t *c = s_commands; c->name != NULL; c++)
    {
        if (tok_eq(line, &toks[cmd_tok], c->name))
        {
            c->handler(id, line, toks, n, params_tok);
            return;
        }
    }

    reply_error(id, "unknown_command");
}

// ============================================================
// ラインバッファ
// ============================================================
void JP_AppendChar(char c)
{
    if (c == '\r' || c == '\n')
    {
        if (s_pos > 0)
        {
            s_line[s_pos] = '\0';
            JP_ProcessLine(s_line);
            s_pos = 0;
        }
    }
    else if (s_pos < JP_MAX_CMD - 1)
    {
        s_line[s_pos++] = c;
    }
    else
    {
        // オーバーフロー: 壊れた行を捨ててリセット (次の行から再同期)
        s_pos = 0;
    }
}
