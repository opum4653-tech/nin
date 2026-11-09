#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include <algorithm>
#include <linux/input.h>
#include <linux/uinput.h>

#include "imgui.h"

#define maxE 5
#define maxF 10
#define UNGRAB 0
#define GRAB 1

bool other_touch = false;

static uint32_t orientation = 0;
static float screenHeight = 0, screenWidth = 0;

struct touchObj {
    bool isDown = false;
    int x = 0;
    int y = 0;
    int id = 0;
};

struct targ {
    int fdNum;
    float maxRawX;
    float maxRawY;
};

static struct {
    input_event downEvent[2]{{{}, EV_KEY, BTN_TOUCH,       1},
                             {{}, EV_KEY, BTN_TOOL_FINGER, 1}};
    input_event event[512]{0};
} input;

static targ targF[maxE];
static touchObj Finger[maxE][maxF];
static int fdNum = 0, origfd[maxE], nowfd;
static int s_primaryFdIndex = 0;
static float s_digitizerMaxX = 0.0f, s_digitizerMaxY = 0.0f;

static bool Touch_initialized = false;
static bool Touch_readOnly = false;

static bool checkDeviceIsTouch(int fd, float &outMaxX, float &outMaxY);

static void genRandomString(char *string, int length) {
    int flag, i;
    srand((unsigned) time(NULL) + length);
    for (i = 0; i < length - 1; i++) {
        flag = rand() % 3;
        switch (flag) {
            case 0:
                string[i] = 'A' + rand() % 26;
                break;
            case 1:
                string[i] = 'a' + rand() % 26;
                break;
            case 2:
                string[i] = '0' + rand() % 10;
                break;
            default:
                string[i] = 'x';
                break;
        }
    }
    string[length - 1] = '\0';
}

static void Upload() {
    static bool bTouch = false;
    static bool isFirstDown = true;
    while (bTouch);
    bTouch = true;
    int tmpCnt = 0, tmpCnt2 = 0, i, j;
    for (i = 0; i < fdNum; i++) {
        for (j = 0; j < maxF; j++) {
            if (Finger[i][j].isDown) {
                if (tmpCnt2++ > 10) {
                    goto finish;
                }
                input.event[tmpCnt].type = EV_ABS;
                input.event[tmpCnt].code = ABS_X;
                input.event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;

                input.event[tmpCnt].type = EV_ABS;
                input.event[tmpCnt].code = ABS_Y;
                input.event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;

                input.event[tmpCnt].type = EV_ABS;
                input.event[tmpCnt].code = ABS_MT_POSITION_X;
                input.event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;

                input.event[tmpCnt].type = EV_ABS;
                input.event[tmpCnt].code = ABS_MT_POSITION_Y;
                input.event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;

                input.event[tmpCnt].type = EV_ABS;
                input.event[tmpCnt].code = ABS_MT_TRACKING_ID;
                input.event[tmpCnt].value = Finger[i][j].id;
                tmpCnt++;

                input.event[tmpCnt].type = EV_SYN;
                input.event[tmpCnt].code = SYN_MT_REPORT;
                input.event[tmpCnt].value = 0;
                tmpCnt++;
            }
        }
    }
    finish:
    bool is = false;
    if (tmpCnt == 0) {
        input.event[tmpCnt].type = EV_SYN;
        input.event[tmpCnt].code = SYN_MT_REPORT;
        input.event[tmpCnt].value = 0;
        tmpCnt++;
        if (!isFirstDown) {
            isFirstDown = true;
            input.event[tmpCnt].type = EV_KEY;
            input.event[tmpCnt].code = BTN_TOUCH;
            input.event[tmpCnt].value = 0;
            tmpCnt++;
            input.event[tmpCnt].type = EV_KEY;
            input.event[tmpCnt].code = BTN_TOOL_FINGER;
            input.event[tmpCnt].value = 0;
            tmpCnt++;
        }
    } else {
        is = true;
    }
    input.event[tmpCnt].type = EV_SYN;
    input.event[tmpCnt].code = SYN_REPORT;
    input.event[tmpCnt].value = 0;
    tmpCnt++;

    if (is && isFirstDown) {
        isFirstDown = false;
        write(nowfd, &input, sizeof(struct input_event) * (tmpCnt + 2));
    } else {
        write(nowfd, input.event, sizeof(struct input_event) * tmpCnt);
    }

    bTouch = false;
}

static void *TypeA(void *arg) {
    targ tmp = *(targ *) arg;
    int i = tmp.fdNum;
    float maxRawX = (tmp.maxRawX > 0.0f) ? tmp.maxRawX : 1080.0f;
    float maxRawY = (tmp.maxRawY > 0.0f) ? tmp.maxRawY : 2400.0f;
    int latest = 0;
    input_event inputEvent[64]{0};

    while (Touch_initialized) {
        auto readSize = (int32_t) read(origfd[i], inputEvent, sizeof(inputEvent));
        if (readSize <= 0 || (readSize % sizeof(input_event)) != 0) {
            continue;
        }
        size_t count = size_t(readSize) / sizeof(input_event);
        for (size_t j = 0; j < count; j++) {
            input_event &ie = inputEvent[j];
            if (ie.type == EV_ABS) {
                if (ie.code == ABS_MT_SLOT) {
                    if (ie.value >= 0 && ie.value < maxF) {
                        latest = ie.value;
                    }
                    continue;
                }
                if (ie.code == ABS_MT_TRACKING_ID) {
                    if (ie.value == -1) {
                        Finger[i][latest].isDown = false;
                    } else {
                        Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                        Finger[i][latest].isDown = true;
                    }
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_X) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].x = ie.value;
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_Y) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].y = ie.value;
                    continue;
                }
            }
            if (ie.code == SYN_REPORT) {
                ImGuiIO &io = ImGui::GetIO();
                if (Finger[i][latest].isDown) {
                    // Continuous pure normalized mathematical mapping [0.0 - 1.0]
                    // Completely eliminates digitizer hardware grid differences (1080, 2400, 4096, 32767)
                    float norm_u = std::clamp((float)Finger[i][latest].x / maxRawX, 0.0f, 1.0f);
                    float norm_v = std::clamp((float)Finger[i][latest].y / maxRawY, 0.0f, 1.0f);

                    float dispW = (screenWidth > 0.0f) ? screenWidth : 2400.0f;
                    float dispH = (screenHeight > 0.0f) ? screenHeight : 1080.0f;

                    // Ensure dispW is long edge and dispH is short edge for landscape
                    float longEdge  = std::max(dispW, dispH);
                    float shortEdge = std::min(dispW, dispH);

                    float x = 0.0f, y = 0.0f;
                    switch (orientation) {
                        case 1: // Landscape (standard mobile gaming orientation)
                            x = norm_v * longEdge;
                            y = (1.0f - norm_u) * shortEdge;
                            break;
                        case 2: // Reverse Portrait (180 deg)
                            x = (1.0f - norm_u) * shortEdge;
                            y = (1.0f - norm_v) * longEdge;
                            break;
                        case 3: // Reverse Landscape (270 deg)
                            x = (1.0f - norm_v) * longEdge;
                            y = norm_u * shortEdge;
                            break;
                        case 0: // Portrait
                        default:
                            x = norm_u * shortEdge;
                            y = norm_v * longEdge;
                            break;
                    }

                    x = std::clamp(x, 0.0f, longEdge);
                    y = std::clamp(y, 0.0f, shortEdge);

                    io.MousePos = ImVec2(x, y);
                    io.MouseDown[0] = true;
                } else {
                    io.MouseDown[0] = false;
                }
                if (!Touch_readOnly) {
                    Upload();
                }
                continue;
            }
        }
    }
    return nullptr;
}

bool Touch_Init(int w, int h, uint32_t orientation_, bool readOnly) {
    char temp[128];
    DIR *dir = opendir("/dev/input/");
    if (!dir) return false;
    dirent *ptr = NULL;
    int eventCount = 0;
    while ((ptr = readdir(dir)) != NULL) {
        if (strstr(ptr->d_name, "event"))
            eventCount++;
    }
    closedir(dir);

    struct input_absinfo absX[maxE], absY[maxE];
    int fd, i;
    float maxArea = 0.0f;
    int bestFdIndex = -1;
    fdNum = 0;

    for (i = 0; i <= eventCount; i++) {
        sprintf(temp, "/dev/input/event%d", i);
        fd = open(temp, O_RDWR);
        if (fd < 0) {
            continue;
        }

        float devMaxX = 0.0f, devMaxY = 0.0f;
        if (checkDeviceIsTouch(fd, devMaxX, devMaxY)) {
            int tmp1 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX[fdNum]);
            int tmp2 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY[fdNum]);
            if (tmp1 == 0 && tmp2 == 0) {
                origfd[fdNum] = fd;
                if (!readOnly) {
                    ioctl(fd, EVIOCGRAB, GRAB);
                }

                // Identify primary screen by largest digitizer resolution area
                float area = devMaxX * devMaxY;
                if (area > maxArea) {
                    maxArea = area;
                    bestFdIndex = fdNum;
                    s_digitizerMaxX = devMaxX;
                    s_digitizerMaxY = devMaxY;
                }

                fdNum++;
                if (fdNum >= maxE)
                    break;
            } else {
                close(fd);
            }
        } else {
            close(fd);
        }
    }

    if (fdNum == 0) {
        puts("[-] Failed to find touchscreen device in /dev/input/");
        return false;
    }

    s_primaryFdIndex = (bestFdIndex >= 0) ? bestFdIndex : 0;

    if (!readOnly) {
        struct uinput_user_dev ui_dev;
        nowfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (nowfd <= 0) {
            return false;
        }

        int string_len = rand() % 10 + 5;
        char string[string_len];
        memset(&ui_dev, 0, sizeof(ui_dev));

        genRandomString(string, string_len);
        strncpy(ui_dev.name, string, UINPUT_MAX_NAME_SIZE);

        ui_dev.id.bustype = 0;
        ui_dev.id.vendor = rand() % 10 + 5;
        ui_dev.id.product = rand() % 10 + 5;
        ui_dev.id.version = rand() % 10 + 5;

        ioctl(nowfd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

        ioctl(nowfd, UI_SET_EVBIT, EV_ABS);
        ioctl(nowfd, UI_SET_ABSBIT, ABS_X);
        ioctl(nowfd, UI_SET_ABSBIT, ABS_Y);
        ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
        ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
        ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
        ioctl(nowfd, UI_SET_EVBIT, EV_SYN);
        ioctl(nowfd, UI_SET_EVBIT, EV_KEY);
        ioctl(nowfd, UI_SET_KEYBIT, BTN_TOOL_FINGER);
        ioctl(nowfd, UI_SET_KEYBIT, BTN_TOUCH);

        genRandomString(string, string_len);
        ioctl(nowfd, UI_SET_PHYS, string);

        ui_dev.absmin[ABS_MT_POSITION_X] = 0;
        ui_dev.absmax[ABS_MT_POSITION_X] = (int)s_digitizerMaxX;
        ui_dev.absmin[ABS_MT_POSITION_Y] = 0;
        ui_dev.absmax[ABS_MT_POSITION_Y] = (int)s_digitizerMaxY;
        ui_dev.absmin[ABS_X] = 0;
        ui_dev.absmax[ABS_X] = (int)s_digitizerMaxX;
        ui_dev.absmin[ABS_Y] = 0;
        ui_dev.absmax[ABS_Y] = (int)s_digitizerMaxY;
        ui_dev.absmin[ABS_MT_TRACKING_ID] = 0;
        ui_dev.absmax[ABS_MT_TRACKING_ID] = 65535;
        write(nowfd, &ui_dev, sizeof(ui_dev));

        if (ioctl(nowfd, UI_DEV_CREATE)) {
            return false;
        }
    }

    Touch_initialized = true;
    Touch_readOnly = readOnly;

    pthread_t t;
    for (i = 0; i < fdNum; i++) {
        targF[i].fdNum = i;
        targF[i].maxRawX = (float)absX[i].maximum;
        targF[i].maxRawY = (float)absY[i].maximum;
        pthread_create(&t, NULL, TypeA, &targF[i]);
    }

    ::screenWidth = (float)w;
    ::screenHeight = (float)h; 
    ::orientation = orientation_;

    system("chmod 000 -R /proc/bus/input/* 2>/dev/null");
    return true;
}

void UpdateScreenData(int w, int h, uint32_t orientation_) {
    ::screenWidth = (float)w;
    ::screenHeight = (float)h; 
    ::orientation = orientation_;
}

static bool checkDeviceIsTouch(int fd, float &outMaxX, float &outMaxY) {
    uint8_t propBits[(INPUT_PROP_MAX + 7) / 8]{0};
    bool hasDirect = false;
    if (ioctl(fd, EVIOCGPROP(sizeof(propBits)), propBits) >= 0) {
        if (propBits[INPUT_PROP_DIRECT / 8] & (1 << (INPUT_PROP_DIRECT % 8))) {
            hasDirect = true;
        }
    }

    struct input_absinfo absX{}, absY{};
    bool hasX = (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) == 0 && absX.maximum > 400);
    bool hasY = (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) == 0 && absY.maximum > 400);

    if (hasX && hasY) {
        outMaxX = (float)absX.maximum;
        outMaxY = (float)absY.maximum;
        return true;
    }
    return false;
}

void Touch_Close() {
    if (Touch_initialized) {
        for (int i = 0; i < maxE; ++i) {
            if (origfd[i] > 0) {
                if (!Touch_readOnly)
                    ioctl(origfd[i], EVIOCGRAB, UNGRAB);
                close(origfd[i]);
                origfd[i] = 0;
            }
        }
        if (nowfd > 0) {
            ioctl(nowfd, UI_DEV_DESTROY);
            close(nowfd);
            nowfd = 0;
        }
        fdNum = 0;
        memset(Finger, 0, sizeof(Finger));
        memset(input.event, 0, sizeof(input.event));
        Touch_initialized = false;
    }
}

void Touch_Down(float xt, float yt) {
    static int x, y;
    x = 0, y = 0;
    switch (orientation) {
        case 1: {
            x = screenHeight - yt;
            y = xt;
            break;
        }                          
        case 2: {
            x = screenWidth - xt;
            y = screenHeight - yt;
            break;
        }
        case 3: {
            y = screenWidth - xt;
            x = yt;
            break;
        }
        default: {
            x = xt;
            y = yt;
            break;
        }
    }
    touchObj &touch = Finger[0][9];
    touch.id = 19;
    touch.x = (int)(x * (s_digitizerMaxX / (screenWidth > 0.0f ? screenWidth : 1.0f)));
    touch.y = (int)(y * (s_digitizerMaxY / (screenHeight > 0.0f ? screenHeight : 1.0f)));        
    touch.isDown = true;
    Upload();
}

void Touch_Move(float x, float y) {
    Touch_Down(x, y);
}

void Touch_Up() {
    touchObj &touch = Finger[0][9];
    touch.isDown = false;
    Upload();
}
