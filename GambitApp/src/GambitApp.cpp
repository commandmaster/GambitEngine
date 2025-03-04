#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include "Renderer.h"
#include "Game.h"

#include <filesystem>
#include <iostream>

#include <windows.h>
#include <iostream>
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

void CalculateScreenDPI() {
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        // Get screen resolution in pixels
        int screenWidth = GetDeviceCaps(hdc, HORZRES);
        int screenHeight = GetDeviceCaps(hdc, VERTRES);

        // Get physical screen size in millimeters
        int screenWidthMM = GetDeviceCaps(hdc, HORZSIZE);
        int screenHeightMM = GetDeviceCaps(hdc, VERTSIZE);

        ReleaseDC(nullptr, hdc);

        // Check if physical dimensions are valid
        if (screenWidthMM <= 0 || screenHeightMM <= 0) {
            std::cerr << "Error: Physical screen dimensions are not available." << std::endl;
            return;
        }

        // Convert millimeters to inches
        double screenWidthInches = static_cast<double>(screenWidthMM) / 25.4;
        double screenHeightInches = static_cast<double>(screenHeightMM) / 25.4;

        // Calculate diagonal size in inches
        double diagonalInches = std::sqrt(
            std::pow(screenWidthInches, 2) + std::pow(screenHeightInches, 2)
        );

        // Calculate diagonal resolution in pixels
        double diagonalPixels = std::sqrt(
            std::pow(screenWidth, 2) + std::pow(screenHeight, 2)
        );

        // Calculate DPI
        double dpi = diagonalPixels / diagonalInches;

        // Output results
        std::cout << "Screen Resolution: " << screenWidth << "x" << screenHeight << " pixels\n";
        std::cout << "Physical Size: " << screenWidthInches << "\" x " << screenHeightInches << "\"\n";
        std::cout << "Diagonal Size: " << diagonalInches << "\"\n";
        std::cout << "Calculated DPI: " << dpi << std::endl;
    } else {
        std::cerr << "Error: Failed to retrieve device context." << std::endl;
    }
}

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Gambit Engine";
	spec.CustomTitlebar = true;
	spec.IconPath = "assets/appLogo.png";

	spec.UIScale = static_cast<float>(GetScreenDPI() / 96.0);
    std::cout << GetScreenDPI() << std::endl;

    CalculateScreenDPI();


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