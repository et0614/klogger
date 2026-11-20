#include "usb_comm.h"
#include "json_protocol.h"

#include "mcc_generated_files/usb/usb_device.h"                        // USBDevice_Handle
#include "mcc_generated_files/usb/usb_cdc/usb_cdc_virtual_serial_port.h" // USB_CDCxxx
#include "mcc_generated_files/usb/usb_cdc/usb_cdc.h"                   // USB_CDCRequestHandler
#include "mcc_generated_files/usb/usb_common/usb_core_events.h"       // USB_ClassRequestCallbackRegister
#include "mcc_generated_files/usb/usb_common/usb_core_transfer.h"     // USB_TransferControlDataSet
#include "mcc_generated_files/timer/delay.h"                          // DELAY_milliseconds
#include <avr/io.h>                                                   // USB0 / USB_ENABLE_bm / USB_ATTACH_bm
#include "mcc_generated_files/usb/usb_descriptors.h"                  // descriptorPointers / STRING_xxx
#include <stddef.h>                                                   // wchar_t

// 内部高周波発振器(OSCHF)の自動補正の切り替え (CLKCTRL.OSCHFCTRLA.AUTOTUNE, 保護レジスタ)
static void setOscAutoTune(uint8_t autotune_gc)
{
    uint8_t v = (uint8_t)((CLKCTRL.OSCHFCTRLA & ~CLKCTRL_AUTOTUNE_gm) | autotune_gc);
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, v);
}

// 【エラッタ対策】SET_LINE_CODING/GET_LINE_CODING を偶数アドレスのバッファで処理する。
// MCC の usb_cdc.c は通信設定(usbCDCLineCoding, 7バイト)のアライメントを指定していないため、
// リンク結果によって奇数アドレスに置かれることがある。マルチパケット有効のOUT転送は偶数アドレス
// 必須 (AVR64DU エラッタ 2.8.1) で、スタックは奇数アドレスを ENDPOINT_ALIGN_ERROR で拒否する。
// その結果 SET_LINE_CODING のデータ段階が完了せず、PC側でポートを開く処理がタイムアウトする。
// (c-logger は偶然偶数アドレスだったため動いていた。生成コードを直すとMCC再生成で消えるのでここで対処)
static USB_CDC_LINE_CODING_t s_lineCoding __attribute__((aligned(2))) = {
    .dwDTERate   = 115200,
    .bCharFormat = USB_CDC_LINE_CODING_ONE_STOP_BIT,
    .bParityType = USB_CDC_LINE_CODING_PARITY_NONE,
    .bDataBits   = USB_CDC_LINE_CODING_8_DATA_BITS,
};

// 【製品名】MCC の既定は "CDC Class Demo"。ブラウザの機器の選択画面 (Web Serial) などに出るので機器の名前にする。
// 生成コードは変えず、文字列記述子の並び (メーカー名 → 製品名 → シリアル番号) をここに持ち直して、
// 製品名だけ差し替える。スタックは bLength をたどって次の文字列を探すので、3つを隙間なく並べること。
// シリアル番号は MCC の既定 ("1") のまま (変えると Windows が別の機器として認識し、COM ポート番号が変わる)。
#define PRODUCT_NAME  L"K-Logger"

static struct {
    USB_DESCRIPTOR_HEADER_t manufacturer_header;
    wchar_t manufacturer[DESCRIPTOR_STRING_LENGTH(STRING_MANUFACTURER)];
    USB_DESCRIPTOR_HEADER_t product_header;
    wchar_t product[DESCRIPTOR_STRING_LENGTH(PRODUCT_NAME)];
    USB_DESCRIPTOR_HEADER_t serial_header;
    wchar_t serial[DESCRIPTOR_STRING_LENGTH(STRING_SERIAL)];
} s_strings = {
    .manufacturer_header = {
        .bLength = sizeof(s_strings.manufacturer) + sizeof(USB_DESCRIPTOR_HEADER_t),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    },
    .manufacturer = STRING_MANUFACTURER,
    .product_header = {
        .bLength = sizeof(s_strings.product) + sizeof(USB_DESCRIPTOR_HEADER_t),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    },
    .product = PRODUCT_NAME,
    .serial_header = {
        .bLength = sizeof(s_strings.serial) + sizeof(USB_DESCRIPTOR_HEADER_t),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    },
    .serial = STRING_SERIAL,
};

static RETURN_CODE_t lineCodingRequestHandler(USB_SETUP_REQUEST_t *req)
{
    if (USB_REQUEST_RECIPIENT_INTERFACE == (USB_REQUEST_RECIPIENT_t)req->bmRequestType.recipient &&
        USB_REQUEST_TYPE_CLASS == (USB_REQUEST_TYPE_t)req->bmRequestType.type &&
        (USB_CDC_REQUEST_SET_LINE_CODING == req->bRequest || USB_CDC_REQUEST_GET_LINE_CODING == req->bRequest))
    {
        return USB_TransferControlDataSet((uint8_t *)&s_lineCoding, sizeof(s_lineCoding), NULL);
    }
    return USB_CDCRequestHandler(req); // その他(SET_CONTROL_LINE_STATE等)は従来どおり
}

void USB_Comm_Initialize(void)
{
    // USB デバイス層 (USBDevice_Initialize) は SYSTEM_Initialize() 済み。
    // ここでは CDC 仮想シリアルポート層のみ初期化する。
    descriptorPointers.stringPtrs[0] = &s_strings.manufacturer_header; // 製品名を "K-Logger" に (ホストが文字列を読むのは attach の100ms以上後)
    USB_CDCVirtualSerialPortInitialize();
    USB_ClassRequestCallbackRegister(lineCodingRequestHandler); // CDCの要求処理を上書き登録

    // USB使用時は OSCHF を USB の SOF(1ms周期) に自動補正する (データシート 12.3.5 で必須)。
    // MCC のクロック設定は AUTOTUNE=OFF のため、補正なしでは発振器の個体差がそのまま残り、
    // USB(許容±0.25%)の通信が成立しない場合がある(列挙はできてもポートが開けない等)。
    setOscAutoTune(CLKCTRL_AUTOTUNE_SOF_gc);
}

void USB_Comm_Task(void)
{
    // 1. USB スタックを駆動 (エニュメレーション応答・TX/RX の進行)
    USBDevice_Handle();
    USB_CDCVirtualSerialPortHandler();

    // 2. 受信バッファをドレインして 1 文字ずつコマンドパーサに渡す。
    //    while で空になるまで読み切ることで 1 tick の受信スループットを稼ぐ。
    uint8_t ch;
    while (USB_CDCRead(&ch) == CDC_SUCCESS)
    {
        JP_AppendChar((char)ch);
    }
}

// 送信バッファが空(=直前の転送が完了し head/tail がリセット済み)になるまで
// USB スタックを駆動して待つ。
//
// 【なぜ必要か】CDC 送信バッファ(usb_cdc_virtual_serial_port.c)は循環バッファを
// 「リニア利用」している: USB_CDCWrite が head を進め、Handler が head バイトを
// 一括送出し、完了コールバックで head=tail=0 にリセットする。このため【転送中
// (in-flight)に USB_CDCWrite で追記すると、完了時の head リセットで追記分が丸ごと
// 破棄される(=バイト欠落)】。これを避けるため、追記の前に必ずバッファが空
// (CIRCBUF_Empty: head==tail)になるまで待ち、「転送中は追記しない」を保証する。
// @return true:空になった / false:budget_ms 内に空にならなかった(ホスト未読など)
static bool wait_tx_drained(int budget_ms)
{
    while (!CIRCBUF_Empty(&usbCDCTransmitBuffer))
    {
        USB_Comm_Pump();
        if (budget_ms-- <= 0) return false;
        DELAY_milliseconds(1);
    }
    return true;
}

// データを「バッファが空になるまで待つ→満杯まで詰める→転送開始」の繰り返しで送る。
// 転送中の追記を一切行わないのでバイト欠落しない。TXバッファ(512B)単位で送出するため
// 大きなデータでも往復回数は少ない (例: 4.4KB ≒ 9 往復)。
// @return 実際に送信できたバイト数
static uint16_t send_block(const uint8_t *data, uint16_t len)
{
    uint16_t sent = 0;
    while (sent < len)
    {
        // 直前の転送完了を待つ(ここで追記しない=欠落防止の肝)
        if (!wait_tx_drained(2000)) break;

        // 空いたバッファに入るだけ詰める (満杯=BUFFER_FULL で停止)
        while (sent < len && USB_CDCWrite(data[sent]) == CDC_SUCCESS)
            sent++;

        // この塊の転送を開始 (Handler が head バイトを WriteStart)
        USB_Comm_Pump();
    }
    // 最後の塊を確実にホストへ送出 (次の送信が転送中追記で潰さないよう空を待つ)
    wait_tx_drained(2000);
    return sent;
}

uint16_t USB_Comm_SendString(const char *str)
{
    uint16_t n = 0;
    while (str[n]) n++;
    return send_block((const uint8_t *)str, n);
}

void USB_Comm_Flush(void)
{
    // 送信バッファの中身を確実にホストへ押し出す (STANDBYスリープ移行直前など、
    // 応答を出してから寝るケースで使う)。USBスタックを少し多めに駆動して TX を drain する。
    for (int i = 0; i < 50; i++)
    {
        USB_CDCVirtualSerialPortHandler();
        DELAY_milliseconds(1);
    }
}

void USB_Comm_Pump(void)
{
    // USBスタック全体を1回駆動する。長時間のバルク送信(dump)中も、デバイスコア
    // (バスイベント/EP0応答)とCDC送出を進めることで、「デバイス無応答化によるホスト側
    // パイプ停止(転送が途中で止まる)」を防ぐ。
    USBDevice_Handle();
    USB_CDCVirtualSerialPortHandler();
}

uint16_t USB_Comm_SendBytes(const uint8_t *data, uint16_t len)
{
    // 生バイナリ送信 (0x00 を含むデータOK)。dump のバルク転送用。
    // 文字列送信と同じ欠落しない送出経路 (send_block) を使う。
    return send_block(data, len);
}

void USB_Comm_Disable(void)
{
    // ロギング(省電力)移行時にUSBモジュールを完全停止する。
    // 【なぜ】USBが有効(USBEN=1)だと主発振 OSCHF(12MHz) をSTANDBY中も要求し続け、MCUが
    // 実質スリープできず ~2mA 消費する(実測)。detach + USBEN=0 で OSCHF が STANDBY で
    // 停止できるようにし、ベース電流を大幅に下げる。
    // 【前提】最後の応答は呼び出し側が USB_Comm_Flush() 済みであること(これ以降USB送信不可)。
    // 【復帰】USB通信に戻すにはリセット(SYSTEM_Initialize が USB を再有効化)。
    setOscAutoTune(CLKCTRL_AUTOTUNE_OFF_gc); // SOFが来なくなるので自動補正を止める(直近の補正値は保持される)
    USB0.CTRLB &= ~USB_ATTACH_bm;   // バスから detach (ホストには切断として見える)
    USB0.CTRLA &= ~USB_ENABLE_bm;   // USBモジュール無効化 → OSCHFクロック要求を解除
}
