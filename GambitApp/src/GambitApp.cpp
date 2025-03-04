#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include "Renderer.h"
#include "Game.h"

#include <filesystem>
#include <iostream>

#include <windows.h>
#include <cmath>

double GetScreenDPI() {
    // Get screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Get physical size in millimeters
    HDC screen = GetDC(NULL);
    int screenWidthMM = GetDeviceCaps(screen, HORZSIZE);
    int screenHeightMM = GetDeviceCaps(screen, VERTSIZE);
    ReleaseDC(NULL, screen);
    
    // Convert mm to inches (1 inch = 25.4 mm)
    double screenWidthInches = screenWidthMM / 25.4;
    double screenHeightInches = screenHeightMM / 25.4;
    
    // Calculate diagonal size in inches
    double diagonalSizeInches = std::sqrt(screenWidthInches * screenWidthInches + screenHeightInches * screenHeightInches);
    
    // Calculate diagonal resolution in pixels
    double diagonalResolution = std::sqrt(screenWidth * screenWidth + screenHeight * screenHeight);
    
    // Calculate DPI
    return diagonalResolution / diagonalSizeInches;
}

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	Walnut::ApplicationSpecification spec;
	spec.Name = "Gambit Engine";
	spec.CustomTitlebar = true;
	spec.IconPath = "assets/appLogo.png";

    spec.UIScale = 0.6f * (GetScreenDPI() / 96.f);

	Walnut::Application* app = new Walnut::Application(spec);


	std::shared_ptr<GameLayer> mainLayer = std::make_shared<GameLayer>("Chess Game");
	app->PushLayer(mainLayer);
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}