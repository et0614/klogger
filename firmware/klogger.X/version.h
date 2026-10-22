
#ifndef VERSION_H
#define VERSION_H

#define DEVICE_NAME      "K-Logger" // 機器名 (hello 応答)
#define FW_VERSION       "3.1.0"    // セマンティックバージョン。K-Logger v3 は 3.x.x
#define PROTOCOL_VERSION 1          // JSONプロトコル世代(互換性が壊れる変更で上げる)

// ビルド日時 (コンパイル時に自動設定。「いつ時点のファームか」の判別用)
// 注: json_protocol.c が再コンパイルされた時刻になるため、正確な刻印にはクリーンビルド推奨。
#define FW_BUILD_DATE    __DATE__
#define FW_BUILD_TIME    __TIME__

#endif /* VERSION_H */
