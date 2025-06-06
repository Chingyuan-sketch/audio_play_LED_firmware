#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define LED_COUNT 8
#define DELAY_US 200000
#define MODE_FILE "/tmp/ws2812_mode.txt"

volatile sig_atomic_t running = 1;
int fd_global = -1;

void handle_signal(int sig) {
    running = 0;
    if (fd_global >= 0) {
        unsigned char buf[LED_COUNT * 3] = {0};
        write(fd_global, buf, sizeof(buf));
        close(fd_global);
    }
    printf("\n收到 signal %d，LED 已清除，退出程式\n", sig);
    exit(0);
}

int read_mode_from_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return -1;
    int mode = -1;
    fscanf(file, "%d", &mode);
    fclose(file);
    return mode;
}

void mode0_step(unsigned char *buf, int fd, int *step) {
    for (int i = 0; i < LED_COUNT; i++) {
        buf[i * 3 + 0] = 20;
        buf[i * 3 + 1] = 0;
        buf[i * 3 + 2] = 0;
    }
    write(fd, buf, LED_COUNT * 3);
}

void mode1_step(unsigned char *buf, int fd, int *step) {
    for (int i = 0; i < LED_COUNT * 3; i++) {
        buf[i] = (*step % 2 == 0) ? 20 : 0;
    }
    write(fd, buf, LED_COUNT * 3);
    (*step)++;
}

void mode2_step(unsigned char *buf, int fd, int *step) {
    memset(buf, 0, LED_COUNT * 3);
    int pos = (*step) % LED_COUNT;
    buf[pos * 3 + 0] = 20;
    buf[pos * 3 + 1] = 0;
    buf[pos * 3 + 2] = 0;
    write(fd, buf, LED_COUNT * 3);
    (*step)++;
}

void mode3_step(unsigned char *buf, int fd, int *step) {
    for (int i = 0; i < LED_COUNT; i++) {
        int on = ((*step % 2) == (i % 2));
        buf[i * 3 + 0] = on ? 20 : 0;
        buf[i * 3 + 1] = on ? 20 : 0;
        buf[i * 3 + 2] = on ? 20 : 0;
    }
    write(fd, buf, LED_COUNT * 3);
    (*step)++;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int fd = open("/dev/ws2812", O_WRONLY);
    if (fd < 0) {
        perror("open /dev/ws2812");
        return 1;
    }
    fd_global = fd;

    unsigned char buf[LED_COUNT * 3];
    int mode = -1, prev_mode = -1;
    int step0 = 0, step1 = 0, step2 = 0, step3 = 0;

    while (running) {
        mode = read_mode_from_file(MODE_FILE);
        if (mode != prev_mode) {
            memset(buf, 0, sizeof(buf));
            write(fd, buf, sizeof(buf));  // 關閉所有 LED
            prev_mode = mode;

            step0 = step1 = step2 = step3 = 0;
        }

        switch (mode) {
            case 0: mode0_step(buf, fd, &step0); break;
            case 1: mode1_step(buf, fd, &step1); break;
            case 2: mode2_step(buf, fd, &step2); break;
            case 3: mode3_step(buf, fd, &step3); break;
            default:
                usleep(100000);
                continue;
        }

        usleep(DELAY_US);
    }

    // 若正常退出
    unsigned char clear[LED_COUNT * 3] = {0};
    write(fd, clear, sizeof(clear));
    close(fd);
    return 0;
}
