#include <LVTask.hpp>

#ifdef _LVTASK_HPP

#define VALIDATE_QUEUE if (!hasValidQueue) { \
                            log_e("No Queue was cerated. Cannot queue command!");\
                            return;\
                        }


LVTask::LVTask(int runningCore, int taskPriority, int stackSize, func_t loopFuncktion)
{
    ss = stackSize;
    lf = loopFuncktion;
    rc = runningCore;
    tp = taskPriority;
}

LVTask::~LVTask() {
    if (instanceRunning) {
        end();
    }
}

bool LVTask::commandValid(int commands[], int cmdCnt, int command) {
    log_v("Validating commad %d in array of %d elements", command, cmdCnt);

    for (int c = 0 ; c < cmdCnt ; c++) {
        if (commands[c] == command) {
            return true;
        }
    }

    return false;
}


void LVTask::begin()
{
    queueUITask = xQueueCreate(10, sizeof(TaskMessageQueue));
    if (queueUITask == 0)
    {
        log_e("Failed to create task queue= %p\n", queueUITask);
        hasValidQueue = false;
        return;
    }

    hasValidQueue = true;
    log_v("Queue item size: %d bytes", sizeof(TaskMessageQueue));

#ifdef ESP32
    if (rc == LV_TASK_RUNNING_CORE_0 || rc == LV_TASK_RUNNING_CORE_1) {
        xTaskCreatePinnedToCore(
            this->uiWorker,     /* Task function. */
            "UI_Task",          /* String with name of task. */
            ss,                 /* Stack size in bytes. */
            this,               /* Parameter passed as input of the task */
            tp,                  /* Priority of the task. */
            &uiWorkerTaskHandle, /* Task handle. */
            rc
        );
    }
    else {
        if (rc != LV_TASK_RUNNING_CORE_NA) {
            log_w("Invalid running core specified for LVTask. Will use default LV_TASK_RUNNING_CORE_NA.");
            rc = LV_TASK_RUNNING_CORE_NA;
        }
#endif
        xTaskCreate(
            this->uiWorker,     /* Task function. */
            "UI_Task",          /* String with name of task. */
            ss,                 /* Stack size in bytes. */
            this,               /* Parameter passed as input of the task */
            tp,                  /* Priority of the task. */
            &uiWorkerTaskHandle /* Task handle. */
        );
#ifdef ESP32
    }
#endif
    instanceRunning = true;
}

void LVTask::end()
{
    TaskMessageQueue taskQueue;

    vTaskDelete(uiWorkerTaskHandle);
    hasValidQueue = false;

    while (xQueueReceive(queueUITask, &taskQueue, (TickType_t)1)) {
        if (taskQueue.param != NULL) {
            free(taskQueue.param);
        }
    }
    vQueueDelete(queueUITask);

    instanceRunning = false;
}

/**
* Adds a task to the LVGL task runner. This can be done form any task. The actual work will be executed by the task also responsible for calliing the lv_timer_handle function.
*
* @param cmd The command to be executed. LV_TASK_...
* @param obj The LVGL object
* @param callBack Optional function to be called when the command was executed by the LVGL API
*/
void LVTask::cmd(int cmd, void *obj, func_t callBack)
{
    int validCommamds[] = {LV_TASK_DISP_LOAD_SCR, LV_TASK_OBJ_CLEAN, LV_TASK_REFRESH_CHART};

    if (!commandValid(validCommamds, sizeof(validCommamds) / sizeof(validCommamds[0]), cmd)) {
        log_e("Command %d is not valid with parameter signature (void*)", cmd);
        return;
    }

    VALIDATE_QUEUE

    TaskMessageQueue txMsg;
    txMsg.cmd = cmd;
    txMsg.obj = obj;
    txMsg.cb = callBack;
    txMsg.value = 0;

    if (xQueueSend(queueUITask, &txMsg, (TickType_t)0) != pdTRUE) {
        log_e("Queue full");
    }
}

/**
* Adds a task to the LVGL task runner. This can be done form any task. The actual work will be executed by the task also responsible for calliing the lv_timer_handle function.
*
* @param cmd The command to be executed. LV_TASK_...
* @param obj The LVGL object
* @param parm The parameter passed to the LVGL API function
* @param callBack Optional function to be called when the command was executed by the LVGL API
*/
void LVTask::cmd(int cmd, void *obj, const char *param, func_t callBack)
{
    int validCommamds[] = {LV_TASK_UPDATE_LABEL};

    if (!commandValid(validCommamds, sizeof(validCommamds) / sizeof(validCommamds[0]), cmd)) {
        log_e("Command %d is not valid with parameter signature (void*, const char*)", cmd);
        return;
    }

    VALIDATE_QUEUE

    TaskMessageQueue txMsg;
    txMsg.cmd = cmd;
    txMsg.obj = obj;
    txMsg.cb = callBack;
    txMsg.param = (char *)malloc(strlen(param) + 1);
    if (txMsg.param == NULL) {
        log_e("Falied to allocate memory for UI label update.");
        return;
    }
    strcpy(txMsg.param, param);
    txMsg.value = 0;

    if (xQueueSend(queueUITask, &txMsg, (TickType_t)0) != pdTRUE) {
        log_e("Queue full");
    }
}

/**
* Adds a task to the LVGL task runner. This can be done form any task. The actual work will be executed by the task also responsible for calliing the lv_timer_handle function.
*
* @param cmd The command to be executed. LV_TASK_...
* @param obj The LVGL object
* @param parm The parameter passed to the LVGL API function
* @param callBack Optional function to be called when the command was executed by the LVGL API
*/
void LVTask::cmd(int cmd, void *obj, int param, func_t callBack)
{
    int validCommamds[] = {LV_TASK_ADD_FLAG, LV_TASK_CLEAR_FLAG, LV_TASK_ADD_STATE, LV_TASK_CLEAR_STATE, LV_TASK_SET_BORDER_WIDTH};

    if (!commandValid(validCommamds, sizeof(validCommamds) / sizeof(validCommamds[0]), cmd)) {
        log_e("Command %d is not valid with parameter signature (void*, int)", cmd);
        return;
    }

    VALIDATE_QUEUE

    TaskMessageQueue txMsg;
    txMsg.cmd = cmd;
    txMsg.obj = obj;
    txMsg.cb = callBack;
    txMsg.value = param;

    if (xQueueSend(queueUITask, &txMsg, (TickType_t)0) != pdTRUE) {
        log_e("Queue full");
    }
}

/**
* Adds a task to the LVGL task runner. This can be done form any task. The actual work will be executed by the task also responsible for calliing the lv_timer_handle function.
*
* @param cmd The command to be executed. LV_TASK_...
* @param obj The LVGL object
* @param obj2 The object passed to the LVGL API function
* @param callBack Optional function to be called when the command was executed by the LVGL API
*/
void LVTask::cmd(int cmd, void *obj, void *obj2, func_t callBack)
{
    int validCommamds[] = {LV_TASK_IMG_SET_SOURCE};

    if (!commandValid(validCommamds, sizeof(validCommamds) / sizeof(validCommamds[0]), cmd)) {
        log_e("Command %d is not valid with parameter signature (void*, void*)", cmd);
        return;
    }

    VALIDATE_QUEUE

    TaskMessageQueue txMsg;
    txMsg.cmd = cmd;
    txMsg.obj = obj;
    txMsg.obj2 = obj2;
    txMsg.cb = callBack;
    txMsg.value = 0;

    if (xQueueSend(queueUITask, &txMsg, (TickType_t)0) != pdTRUE) {
        log_e("Queue full");
    }
}

/**
* Adds a task to the LVGL task runner. This can be done form any task. The actual work will be executed by the task also responsible for calliing the lv_timer_handle function.
*
* @param cmd The command to be executed. LV_TASK_...
* @param function The function to be called from the LVTask task runner
* @param fp The parameter passed to the called function
* @param callBack Optional function to be called when the command was executed by the LVTask runner
*/
void LVTask::cmd(int cmd, func_p_t function, void *fp, func_t callBack)
{
    int validCommamds[] = {LV_TASK_EXECUTE_FUNCTION};

    if (!commandValid(validCommamds, sizeof(validCommamds) / sizeof(validCommamds[0]), cmd)) {
        log_e("Command %d is not valid with parameter signature (func_t)", cmd);
        return;
    }

    VALIDATE_QUEUE

    TaskMessageQueue txMsg;
    txMsg.cmd = cmd;
    txMsg.obj = NULL;
    txMsg.cb = callBack;
    txMsg.func = function;
    txMsg.fparam = fp;
    txMsg.value = 0;

    if (xQueueSend(queueUITask, &txMsg, (TickType_t)0) != pdTRUE) {
        log_e("Queue full");
    }
}

void LVTask::uiWorker(void *tParams)
{
    LVTask *_this = static_cast<LVTask *>(tParams);
    TaskMessageQueue rxQueue;
    func_t postTaskCB = NULL;
    unsigned long workTimer = 0;

    while (true)
    {
        if (xQueueReceive(_this->queueUITask, &rxQueue, (TickType_t)1))
        {
            switch (rxQueue.cmd)
            {
#if LV_USE_LABEL                
            case LV_TASK_UPDATE_LABEL:
                lv_label_set_text((lv_obj_t *)rxQueue.obj, rxQueue.param);
                free(rxQueue.param);
                break;
#endif
            case LV_TASK_DISP_LOAD_SCR:
                lv_disp_load_scr((lv_obj_t *)rxQueue.obj);
                break;

            case LV_TASK_ADD_FLAG:
                lv_obj_add_flag((lv_obj_t *)rxQueue.obj, rxQueue.value);
                break;

            case LV_TASK_CLEAR_FLAG:
                lv_obj_clear_flag((lv_obj_t *)rxQueue.obj, rxQueue.value);
                break;

            case LV_TASK_ADD_STATE:
                lv_obj_add_state((lv_obj_t *)rxQueue.obj, rxQueue.value);
                break;

            case LV_TASK_CLEAR_STATE:
                lv_obj_clear_state((lv_obj_t *)rxQueue.obj, rxQueue.value);
                break;
#if LV_USE_IMG
            case LV_TASK_IMG_SET_SOURCE:
                lv_img_set_src((lv_obj_t *)rxQueue.obj, (const void *)rxQueue.obj2);
                break;
#endif
            case LV_TASK_OBJ_CLEAN:
                lv_obj_clean((lv_obj_t *)rxQueue.obj);
                break;

            case LV_TASK_SET_BORDER_WIDTH:
                lv_obj_set_style_border_width((lv_obj_t *)rxQueue.obj, rxQueue.value, LV_PART_MAIN| LV_STATE_DEFAULT);
                break;

#if LV_USE_CHART
            case LV_TASK_REFRESH_CHART:
                lv_chart_refresh((lv_obj_t *)rxQueue.obj);
                break;
#endif

            case LV_TASK_EXECUTE_FUNCTION:
                rxQueue.func(rxQueue.fparam);
                break;

            default:
                log_e("UI task, invalid command");
                break;
            }

            postTaskCB = rxQueue.cb;
        }

        if (millis() >= workTimer) {
            workTimer = lv_timer_handler() + millis();
        }

        if (postTaskCB != NULL)
        {
            postTaskCB();
            postTaskCB = NULL;
        }

        if (_this->lf != NULL)
        {
            _this->lf();
        }

        vTaskDelay(1);
    }
}

#endif
