/**
 @version 1.2.0
*/


#if (__has_include(<lvgl.h>) || __has_include("lvgl.h")) && __has_include(<Arduino.h>)

#ifndef _LVTASK_HPP
#define _LVTASK_HPP

#include <Arduino.h>
#include <lvgl.h>

#define LV_TASK_UPDATE_LABEL 1 /* cmd(LV_TASK_UPDATE_LABEL, lv_label_obj, char*, [CB func]) */
#define LV_TASK_DISP_LOAD_SCR 2
#define LV_TASK_ADD_FLAG 3
#define LV_TASK_CLEAR_FLAG 4
#define LV_TASK_ADD_STATE 5
#define LV_TASK_CLEAR_STATE 6
#define LV_TASK_IMG_SET_SOURCE 7
#define LV_TASK_OBJ_CLEAN 8
#define LV_TASK_SET_BORDER_WIDTH 9
#define LV_TASK_REFRESH_CHART 10
#define LV_TASK_EXECUTE_FUNCTION 100

#define LV_TASK_RUNNING_CORE_NA -1
#define LV_TASK_RUNNING_CORE_0 0
#define LV_TASK_RUNNING_CORE_1 1

class LVTask
{
private:
    typedef void (*func_t)();
    typedef void (*func_p_t)(void *);

    struct TaskMessageQueue
    {
        int cmd;
        void *obj;
        void *obj2;
        char *param = NULL;
        int value;
        func_t cb;
        func_p_t func;
        void *fparam;
    };

    int ss = 8192;
    func_t lf = NULL;
    int rc = LV_TASK_RUNNING_CORE_NA;
    int tp = 2;
    bool hasValidQueue = false;
    QueueHandle_t queueUITask;
    static void uiWorker(void *);
    TaskHandle_t uiWorkerTaskHandle = NULL;
    bool instanceRunning = false;
    bool commandValid(int commands[], int cmdCnt, int command);

public:
    LVTask(int runningCore = LV_TASK_RUNNING_CORE_NA, int taskPriority = 2, int stackSize = 8192, func_t loopFunction = NULL);
    ~LVTask();
    void begin();
    void end();
    void cmd(int cmd, void *obj, func_t callBack = NULL);
    void cmd(int cmd, void *obj, const char *param, func_t callBack = NULL);
    void cmd(int cmd, void *obj, int param, func_t callBack = NULL);
    void cmd(int cmd, void *obj, void *obj2, func_t callBack = NULL);
    void cmd(int cmd, func_p_t function, void *fp, func_t callBack = NULL);
};

#endif

#else
#error "This library is to be used togheter with the UI library LVGL and the Arduino framework."

#endif // has lvgl.h and Arduino.h

