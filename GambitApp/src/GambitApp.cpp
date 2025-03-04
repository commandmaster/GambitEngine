#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include "Renderer.h"
#include "Game.h"

#include <filesystem>
#include <iostream>

#include <windows.h>

float GetScreenScaleFactor() {
    HDC screen = GetDC(NULL);
    int dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    return dpi / 96.0f; 
}

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Gambit Engine";
	spec.CustomTitlebar = true;
	spec.IconPath = "assets/appLogo.png";

	spec.UIScale = GetScreenScaleFactor();
	std::cout << GetScreenScaleFactor() << std::endl;


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