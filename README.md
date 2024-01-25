# LVTask
LVTask is a task wrapper for the grapichs library [LVGL](https://lbgl.io) written is c++. This library is designed to be used with the Arduino framework and the ESP32 platform. The library may or may not work with other FreeRTOS based paltforms that uses the Arduino framework. This has not been verified or tested by the author.

## Description
LVTask creates a dedicated task that handles the periodicaly calls to the LVGL API function lv_timer_handle() which is responsible for handleling UI updates, renderings and user input from eg. touch screens. LVGL also implements a command queue and a tool set for the application that may be used for interacting with the LVGL API. These tools may be used accross any task, with no regards to the fact that the LVGL library itself is not thread safe. All submitted command are processed by the same thread that also handles the lv_timer_handle() calls.

LVTask can also be used to call functions, that them self make calls to the non thread safe LVGL library API, from any thread.

## Why
It's no secret that I am a huge fan of Arduino, ESP32 and LVGL. It makes it easy to create impresive devices with nice graphics and UI. The fact that you can use internet recurses via the ESP WiFi makes it even better. For the WiFi setup I tend to use another great library, the WiFiManager. I quickly realized, that for every project, using the mentiond libraries, I ran into two problems. First problem was related to the WiFiManger which is initialized in the setup() function. If the Wifi SSID is not available or if the WiFi is not configured, WiFiManager starts is own web configuration portal, which is great, and one of the main reasons for using it? At the same time it is possible to add a call back function. I use this function to display instructions to the user, using LVGL. The issue thow, is that WiFiManager is esentily blocking the threat in the setup() function. This, ofcurce, means that the loop() is never called, and thus the lv_timer_handle() is never called, again this causes the display not beeing updated and the user will never see the WiFi connect instructions. And yes, I am aware that the WiFiManager can be configured to run in non-blocking mode, and then the loop() will run, and the display updated. 

My agrument here is that skipping throug setup() with no WiFi connection requeres some handeling in the loop() and to my experience this can get a bit messy. Using LVTask, the "main" thred can be blocked without preventing LVGL from updating the display. Ofcurse you could also implement a busy wait loop with in the setup() function and call lv_timer_handle() from there, but why not use FreeRTOS tasks now they are there?

The second problem I ran into was related to the non thread safe LVGL library. Usualy I reley on other 3rd party libraries that uses tasks and provide call back functions to provide data. Sometimes I would need to update the LVGL UI bssed on the data recerived by the cb function. But since this function would run in a different task it would not be safe to call the LVGL API from within. LVTask makes it possible to queue commands, form any task, that eventually will be processed by the LVTask task runner and executed in the same task that also handles the calls to lv_timer_handle() thus not violating the tread safty implementation.



## Dependencies
- Arduino
- FreeRTOS (ESP32 platform)
- LVGL (Version 8.x.x)

## Usage
### Library functions
**begin**
```c
begin();
```
Creates the command queue and the task runner function with default parameters.

**end**
```c
end();
```
Destroys the task runner and the command queue and performes clean up.

**cmd**
```c
cmd(task_command, lvgl_object, [cb_function]);
cmd(task_command, lvgl_object, parameter, [cb_function]);
cmd(task_command, lvgl_object, lvgl_object, [cb_function]);
cmd(task_command, function, parameter, [cb_function]);
```

### Implemented commands
The following list contains the currently implemented commands that can be used to manipulate the LVGL UI and controles. All commands has an optional callback function which, if provided, will be called by the task runner once the command has been parsed to the LVGL API.

**LV_TASK_UPDATE_LABEL** - Updates a LVGL Label.
```c++
cmd(LV_TASK_UPDATE_LABEL, lbHeading, "New heading");
```

**LV_TASK_DISP_LOAD_SCR** - Load a specific screen and makes it visible on the display.
```c++
cmd(LV_TASK_DISP_LOAD_SCR, scrSetup);
```

**LV_TASK_ADD_FLAG** - Adds a flag to a LVGL object.
```c++
cmd(LV_TASK_ADD_FLAG, lbWarning, LV_OBJ_FLAG_HIDDEN);
```

**LV_TASK_CLEAR_FLAG** - Removes / clears a flag from a LVGL object.
```c++
cmd(LV_TASK_CLEAR_FLAG, lbWarning, LV_OBJ_FLAG_HIDDEN);
```

**LV_TASK_ADD_STATE** - Adds a state to a LVGL object.
```c++
cmd(LV_TASK_ADD_STATE, btnSubmit, LV_STATE_DISABLED);
```

**LV_TASK_CLEAR_STATE** - Removes / clears a state to a LVGL object.
```c++
cmd(LV_TASK_CLEAR_STATE, btnSubmit, LV_STATE_DISABLED);
```

**LV_TASK_IMG_SET_SOURCE** - Assigns an image to a specific LVGL image object.
```c++
cmd(LV_TASK_IMG_SET_SOURCE, imgPhoto, &photoData);
```

**LV_TASK_OBJ_CLEAN** - Removes / cleans all chlid objects of the specified object.
```c++
cmd(LV_TASK_OBJ_CLEAN, plBooks);
```

**LV_TASK_SET_BORDER_WIDTH** - Sets the boarder width of a LVGL object.

*Applis to LV_PART_MAIN | LV_STATE_DEFAULT*
```c++
cmd(LV_TASK_SET_BORDER_WIDTH, btnSumbit, 4);
```

**LV_TASK_REFRESH_CHART** - Updates the specified chart with currently provided data.

*It should be safe to use direct LVGL API calls to provide datasets since this will not invalidate any display objects.*
```c++
cmd(LV_TASK_REFRESH_CHART, crtStatistics);
```

**LV_TASK_EXECUTE_FUNCTION** - Calls a user function from the LVGL worker task. 

*This can be used to invvoke a function implemeted in the app, that uses the LVGL API directly. As this function is run from the LVGL worker task, it will prevent display updates for as long as it runs, so take care not to implement blocking- or long runnig code.*
```c++
cmd(LV_TASK_EXECUTE_FUNCTION, function, (void *)function_parameter);
```

### Example code

```c++
#include <Arduino.h>
#include <lvgl.h>
#include <LVTask.hpp>
#include <Ticker.h>

// Global variables - accessable to all tasks.
LVTask lvTask = LVTask();
Ticker ticker1s;
int cnt, cnt2 = 0;

lv_obj_t *ui_scrMain;
lv_obj_t *ui_lbTempTxt;
lv_obj_t *ui_lbTempTxt2;

void ticker1sHandler() {
    // This code is run from the Ticker task and thus it would not be safe to call the LVGL API directly from here.
    char buf[20];
    sprintf(buf, "Counter = %d", cnt);
    lvTask.cmd(LV_TASK_UPDATE_LABEL, ui_lbTempTxt, buf); // Queue command for the LVGL task runner.
    cnt++;
}

void taskFunction(void *parameter) {
    // This code is run from the LVGL worker task and thus it would be safe to call the LVGL API from here.
    // NOTE: Blocking- or long running code will prevent the display from being updated and prevent touch screen input.
    int value = *(int *)parameter;
    char buf[20];
    sprintf(buf, "Counter2 = %d", value);

    lv_label_set_text(ui_lbTempTxt2, buf); // Direct LVGL API call.
}

void setup() {
    Serial.begin(115200);

    /*
        Initialize display H/W here.
    */

   /*
        Initialize LVGL here.
   */

    // Add a screen and 2 labels to the display.
    // It is safe call the LVGL API in setup() because we have not yet started the task worker.
    // Also, if using SquareLine Studio for UI building, it is safe to call ui_init() in setup() before calling lvTask.begin()
    ui_scrMain = lv_obj_create(NULL);

    ui_lbTempTxt = lv_label_create(ui_scrMain);
    lv_obj_set_width(ui_lbTempTxt, LV_SIZE_CONTENT); 
    lv_obj_set_height(ui_lbTempTxt, LV_SIZE_CONTENT); 
    lv_obj_set_x(ui_lbTempTxt, 0);
    lv_obj_set_y(ui_lbTempTxt, 0);
    lv_label_set_text(ui_lbTempTxt, "Initialized");
    lv_obj_set_style_text_font(ui_lbTempTxt, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_lbTempTxt2 = lv_label_create(ui_scrMain);
    lv_obj_set_width(ui_lbTempTxt2, LV_SIZE_CONTENT); 
    lv_obj_set_height(ui_lbTempTxt2, LV_SIZE_CONTENT); 
    lv_obj_set_x(ui_lbTempTxt2, 0);
    lv_obj_set_y(ui_lbTempTxt2, 20);
    lv_label_set_text(ui_lbTempTxt2, "Initialized");
    lv_obj_set_style_text_font(ui_lbTempTxt2, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lvTask.begin(); // start a dedicated task to service the LVGL library and API.
    ticker1s.attach(1, ticker1sHandler); // Call ticker1sHandler() every second.
}

void loop() {
    // Even though the "main" task is bloked, the display will still be updated once a second by the Ticker.
    lvTask.cmd(LV_TASK_EXECUTE_FUNCTION, taskFunction, &cnt2);
    cnt2++; // Keep in mind that we are using pointers. So changing the value of cnt2 before the command has been processed will affect the value read by the called function.
    delay(10000); // Block "main" thread for 10 seconds.
}
```

# License
MIT licence

Copyright (c) 2024 - Thomas Houlberg