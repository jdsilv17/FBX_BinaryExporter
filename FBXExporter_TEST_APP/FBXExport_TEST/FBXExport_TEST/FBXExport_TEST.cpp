// FBXExport_TEST.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include "FBX_Export_Interface.h"

std::string replaceExt(std::string& s, const std::string& newExt);

int main()
{
	//std::cout << Get_Scene_Poly_Count("BattleMage.fbx") << " Polygons in Mesh";

	for (auto& entry : std::filesystem::directory_iterator("./"))
	{
		if (entry.path().extension() == ".fbx")
		{
			int result = -1;
			std::string newFileName = entry.path().string();
			replaceExt(newFileName, "mesh");
			result = export_simple_mesh(entry.path().string().c_str(), newFileName.c_str());
			if (result == 0)
				std::cout << newFileName + " exported SUCCESSFULLY" << std::endl;
			else
				std::cout << newFileName + " did NOT export successfully" << std::endl;

			newFileName = entry.path().string();
			replaceExt(newFileName, "mats");
			result = export_materials(entry.path().string().c_str(), newFileName.c_str());
			if (result == 0)
				std::cout << newFileName + " exported SUCCESSFULLY" << std::endl;
			else
				std::cout << newFileName + " did NOT export successfully" << std::endl;

			newFileName = entry.path().string();
			replaceExt(newFileName, "anim");
			result = export_animation(entry.path().string().c_str(), newFileName.c_str());
			if (result == 0)
				std::cout << newFileName + " exported SUCCESSFULLY" << std::endl;
			else
				std::cout << newFileName + " did NOT export successfully" << std::endl;
		}
	}
	system("pause");
	return 0;
}

std::string replaceExt(std::string& s, const std::string& newExt) {

	std::string::size_type i = s.rfind('.', s.length());

	if (i != std::string::npos) {
		return s.replace(i + 1, newExt.length(), newExt);
	}
	return s;
}
