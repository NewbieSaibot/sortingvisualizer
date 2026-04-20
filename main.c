#ifndef MAIN_C
#define MAIN_C

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "styles/rltech/style_rltech.h"

#include "sortthread.h"
#include "openurl.h"

int main() {
    InitWindow(1280, 720, "Sorting Visualizer");
    SetTargetFPS(GetMonitorRefreshRate(0));

	GuiLoadStyleRLTech();

	GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    NArray* array = NULL;
    narrinit(&array, (1 << 7));
	init_xorshift();

    // narrshuffle(array, time(NULL));
    
    SortData* stats = NULL;
    sdatainit(&stats);
    
    SortThread sorter;
    sthreadinit(&sorter, array, stats);
    sthreadstart(&sorter);

	static int speed = 50000000ll;
	bool spdedtmode = false;

	const float maxcdfrcnxt = 0.01f;
	float cdfrcnxt = 0;

	bool shuffling = false;

	int arraysz = (1 << 7);
	int oldarraysz = (1 << 7);
	bool arredtmode = false;
	bool arrrst = false;

	sthreadsetspeed(&sorter, speed);

	bool rainbow = false;

	int st = 0;
	int prvst = 0;

	const size_t panelw = 175;

    while (!WindowShouldClose()) {
		if (arrrst) {
			sthreadapply(&sorter, (size_t)arraysz, (SortType)st);
			oldarraysz = arraysz;
			prvst = st;
			arrrst = false;
		}

		shuffling = sorter.shuffling;

        if (IsKeyPressed(KEY_SPACE)) {
            if (sthreadiscomplete(&sorter)) continue;
            
            static bool paused = false;
            paused = !paused;
            if (paused) sthreadpause(&sorter);
            else sthreadresume(&sorter);
        }
        
        if (IsKeyDown(KEY_RIGHT) && cdfrcnxt >= maxcdfrcnxt) {
            sthreadforcenxt(&sorter);
			cdfrcnxt = 0;
        }
		cdfrcnxt += 1.f / GetFPS();
        
        if (IsKeyPressed(KEY_C)) {
			rainbow ^= 1;
        }
        
        size_t comparisons, swaps, reads, writes;
        sthreadgetstats(&sorter, &comparisons, &swaps, &reads, &writes);
        bool complete = sthreadiscomplete(&sorter);
        
        BeginDrawing();
        ClearBackground((Color){40, 40, 40, 255});
        
		if (!rainbow) {
			narrdraw(sorter.array, panelw, 0, GetScreenWidth(), GetScreenHeight());
		} else {
			narrdrawrainbow(sorter.array, panelw, 0, GetScreenWidth(), GetScreenHeight());
		}

		DrawRectangle(0, 0, panelw, GetScreenHeight(), GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		if (GuiLabelButton((Rectangle){20, 0, 140, 30}, GuiIconText(ICON_STAR, "Sorting Visualizer")) || 
			GuiLabelButton((Rectangle){20, 30, 140, 30}, "by NewbieSaibot")) {
			open_url("https://github.com/NewbieSaibot/sortingvisualizer");
		}

		GuiLabel((Rectangle){20, 80, 140, 40}, TextFormat("Comparisons: %zu", comparisons));
		GuiLabel((Rectangle){20, 110, 140, 40}, TextFormat("Swaps: %zu", swaps));
		GuiLabel((Rectangle){20, 140, 140, 40}, TextFormat("Reads: %zu", reads));
		GuiLabel((Rectangle){20, 170, 140, 40}, TextFormat("Writes: %zu", writes));
		GuiLabel((Rectangle){20, 200, 140, 40}, TextFormat("Status: %s", (shuffling) ? "SHUFFLING" : (complete ? "DONE" : "SORTING")));

		if (GuiButton((Rectangle){20, 250, 140, 40}, GuiIconText(ICON_BRUSH_PAINTER, "Color"))) {
			rainbow ^= 1;
		}
		static bool paused = false;
		if (GuiButton((Rectangle){20, 300, 140, 40}, TextFormat(!paused ? GuiIconText(ICON_PLAYER_PAUSE, "Pause") : GuiIconText(ICON_PLAYER_PLAY, "Resume")))) {
            if (sthreadiscomplete(&sorter)) goto pauseskip;
            
            paused = !paused;
            if (paused) sthreadpause(&sorter);
            else sthreadresume(&sorter);
		}
pauseskip:
		if (GuiButton((Rectangle){20, 350, 140, 40}, GuiIconText(ICON_RESTART, "Reset / Apply"))) {
			arrrst = 1;
		}

		GuiLabel((Rectangle){20, 410, 140, 40}, GuiIconText(ICON_ARROW_RIGHT, "Speed"));
		if (GuiValueBox((Rectangle){20, 450, 140, 40}, NULL, &speed, 1, 2000000000, spdedtmode)) {
			spdedtmode = !spdedtmode;

			sthreadsetspeed(&sorter, speed);
		}

		GuiLabel((Rectangle){20, 520, 140, 40}, GuiIconText(ICON_RESIZE, "Array size"));
		if (GuiValueBox((Rectangle){20, 560, 140, 40}, NULL, &arraysz, 1, 200000, arredtmode)) {
			arredtmode = !arredtmode;
		}

		GuiLabel((Rectangle){20, 630, 140, 40}, GuiIconText(ICON_CPU, "Sorting algorithm"));
		GuiComboBox((Rectangle){20, 670, 140, 40}, "Bubble;Shaker;Gnome;Shell;Selection;Quick;Merge;Natural Merge;Heap;Bogo", &st);

        EndDrawing();
    }
    
    sthreaddestroy(&sorter);
    narrfree(&sorter.array);
    sdatafree(&stats);
    CloseWindow();
    
    return 0;
}

#endif // MAIN_C
