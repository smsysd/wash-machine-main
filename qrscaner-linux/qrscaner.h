#ifndef QRSCANER_H_
#define QRSCANER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define QRSCANER_AUTO			0xFFFFFFF

int qrscaner_init(const char* camPath, int width, int height, int bright, int expos, int focus, int contrast, int equalBlocking, void (*callback)(const char* qr_code));
void qrscaner_start(); //Запуск
void qrscaner_stop();  //Остановка
void qrscaner_clear();

#ifdef __cplusplus
}
#endif

#endif