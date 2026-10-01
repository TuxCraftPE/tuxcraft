#ifndef MAIN_GLFW_H__
#define MAIN_GLFW_H__

#include "App.h"
#include "GLFW/glfw3.h"
#include "client/renderer/gles.h"
#include "SharedConstants.h"

#include <cstdio>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <ctime>
#include <png.h>
#include <sys/stat.h>
#ifdef _WIN32
#   include <direct.h>
#   define mkdir_compat(p) _mkdir(p)
#else
#   include <unistd.h>
#   define mkdir_compat(p) mkdir(p, 0777)
#endif
#include <fstream>
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "util/Mth.h"
#include "AppPlatform_glfw.h"
#include "world/entity/monster/TuxAssets.h"

static App* g_app = 0;
static bool g_takeScreenshot = false;

static bool savePngImage(const std::string& filepath, int width, int height, const unsigned char* rgbaPixels) {
	FILE* fp = fopen(filepath.c_str(), "wb");
	if (!fp) return false;

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) { fclose(fp); return false; }

	png_infop info = png_create_info_struct(png);
	if (!info) { png_destroy_write_struct(&png, NULL); fclose(fp); return false; }

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		return false;
	}

	png_init_io(png, fp);
	png_set_IHDR(png, info, width, height, 8,
				 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);

	std::vector<png_bytep> rowPointers(height);
	for (int y = 0; y < height; ++y) {
		rowPointers[y] = (png_bytep)(rgbaPixels + (height - 1 - y) * width * 4);
	}
	png_write_image(png, rowPointers.data());
	png_write_end(png, NULL);

	png_destroy_write_struct(&png, &info);
	fclose(fp);
	return true;
}

static void setupLinuxAppIcon() {
#if defined(__linux__) && !defined(STANDALONE_SERVER)
	const char* home = getenv("HOME");
	if (!home) return;

	std::string userShare = std::string(home) + "/.local/share";
	std::string appDir = userShare + "/applications";
	std::string iconDir = userShare + "/icons";
	std::string hicolorDir = iconDir + "/hicolor/256x256/apps";

	mkdir(userShare.c_str(), 0777);
	mkdir(appDir.c_str(), 0777);
	mkdir(iconDir.c_str(), 0777);
	mkdir((iconDir + "/hicolor").c_str(), 0777);
	mkdir((iconDir + "/hicolor/256x256").c_str(), 0777);
	mkdir(hicolorDir.c_str(), 0777);

	std::string iconPath = iconDir + "/tuxcraft.png";
	std::string hicolorPath = hicolorDir + "/tuxcraft.png";
	std::string desktopPath = appDir + "/tuxcraft.desktop";

	std::ifstream testIcon(iconPath.c_str(), std::ios::binary);
	if (!testIcon.is_open()) {
		std::ifstream src("data/images/icon.png", std::ios::binary);
		if (!src) src.open("icon.png", std::ios::binary);
		if (!src) src.open("data/icon.png", std::ios::binary);
		if (src) {
			std::ofstream dst(iconPath.c_str(), std::ios::binary);
			dst << src.rdbuf();
			src.clear();
			src.seekg(0, std::ios::beg);
			std::ofstream dstH(hicolorPath.c_str(), std::ios::binary);
			dstH << src.rdbuf();
		}
	}

	char exePath[1024] = {0};
	ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
	if (len <= 0) {
		strcpy(exePath, "TuxCraft");
	}

	std::ofstream df(desktopPath.c_str());
	if (df) {
		df << "[Desktop Entry]\n"
		   << "Name=TuxCraft\n"
		   << "Comment=TuxCraft Game\n"
		   << "Exec=" << exePath << "\n"
		   << "Icon=" << iconPath << "\n"
		   << "Terminal=false\n"
		   << "Type=Application\n"
		   << "Categories=Game;\n"
		   << "StartupWMClass=tuxcraft\n";
	}
#endif
}

int transformKey(int glfwkey) {
	if (glfwkey >= GLFW_KEY_F1 && glfwkey <= GLFW_KEY_F12) {
		return glfwkey - 178;
	}

	switch (glfwkey) {
		case GLFW_KEY_ESCAPE: return Keyboard::KEY_ESCAPE;
		case GLFW_KEY_BACKSPACE: return Keyboard::KEY_BACKSPACE;
		case GLFW_KEY_LEFT_SHIFT: return Keyboard::KEY_LSHIFT;
		case GLFW_KEY_ENTER: return Keyboard::KEY_RETURN;
		default: return glfwkey;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(action == GLFW_REPEAT) return;

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
		if (g_app) {
			((MAIN_CLASS*)g_app)->options.hideGui = !((MAIN_CLASS*)g_app)->options.hideGui;
		}
		return;
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		g_takeScreenshot = true;
		return;
	}

	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		GLFWmonitor* monitor = glfwGetWindowMonitor(window);
		if (monitor) {
			// Currently fullscreen → go windowed
			glfwSetWindowMonitor(window, NULL, 80, 80, 854, 480, 0);
		} else {
			// Currently windowed → go fullscreen on primary monitor
			GLFWmonitor* primary = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primary);
			glfwSetWindowMonitor(window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		return;
	}

	Keyboard::feed(transformKey(key), action);
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
	Keyboard::feedText(codepoint);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	static double lastX = 0.0, lastY = 0.0;
	static bool firstMouse = true;

	if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

	double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		Mouse::feed(0, 0, xpos, ypos, deltaX, deltaY);
	} else { 
		Mouse::feed( MouseAction::ACTION_MOVE, 0, xpos, ypos);
	}
	Multitouch::feed(0, 0, xpos, ypos, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(action == GLFW_REPEAT) return;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		Mouse::feed( MouseAction::ACTION_LEFT, action, xpos, ypos);
		Multitouch::feed(1, action, xpos, ypos, 0);
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		Mouse::feed( MouseAction::ACTION_RIGHT, action, xpos, ypos);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	Mouse::feed(3, 0, xpos, ypos, 0, yoffset);
}

void window_size_callback(GLFWwindow* window, int width, int height) {
	if (g_app) g_app->setSize(width, height);
}

void error_callback(int error, const char* desc) {
	printf("Error: %s\n", desc);
}

int main(void) {
	AppContext appContext;

#ifndef STANDALONE_SERVER
	// Platform init.
	appContext.platform = new AppPlatform_glfw();

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#ifdef GLFW_WAYLAND_APP_ID
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "tuxcraft");
#endif
#ifdef GLFW_X11_CLASS_NAME
	glfwWindowHintString(GLFW_X11_CLASS_NAME, "tuxcraft");
#endif
#ifdef GLFW_X11_INSTANCE_NAME
	glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "tuxcraft");
#endif

	setupLinuxAppIcon();

	GLFWwindow* window = glfwCreateWindow(appContext.platform->getScreenWidth(), appContext.platform->getScreenHeight(), "TuxCraft", NULL, NULL);
	
	if (window == NULL) {
		return 1;
	}

	int iconW = 0, iconH = 0;
	const void* iconPixels = getTuxIconRgba(&iconW, &iconH);
	if (iconPixels && iconW > 0 && iconH > 0) {
		GLFWimage iconImage;
		iconImage.width = iconW;
		iconImage.height = iconH;
		iconImage.pixels = (unsigned char*)iconPixels;
		glfwSetWindowIcon(window, 1, &iconImage);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, character_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	glfwMakeContextCurrent(window);
	gladLoadGLES1Loader((GLADloadproc)winGLLoader);
	glfwSwapInterval(0);
	glPatchDesktopCompat();
#endif

	App* app = new MAIN_CLASS();

	g_app = app;
	((MAIN_CLASS*)g_app)->externalStoragePath = ".";
	((MAIN_CLASS*)g_app)->externalCacheStoragePath = ".";
	g_app->init(appContext);
	g_app->setSize(appContext.platform->getScreenWidth(), appContext.platform->getScreenHeight());

	// Main event loop
	using clock = std::chrono::steady_clock;
	while(!glfwWindowShouldClose(window) && !app->wantToQuit()) {
		auto frameStart = clock::now();

		app->update();

		if (g_takeScreenshot) {
			g_takeScreenshot = false;
			int fbW = 0, fbH = 0;
			glfwGetFramebufferSize(window, &fbW, &fbH);
			if (fbW > 0 && fbH > 0) {
				std::vector<unsigned char> pixels(fbW * fbH * 4);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadPixels(0, 0, fbW, fbH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

				mkdir_compat("screenshots");

				time_t rawtime;
				struct tm* timeinfo;
				char timeBuf[80];
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d_%H.%M.%S", timeinfo);

				std::string filename = std::string("screenshot_") + timeBuf + ".png";
				std::string filepath = std::string("screenshots/") + filename;

				int counter = 1;
				while (access(filepath.c_str(), F_OK) == 0) {
					filepath = std::string("screenshots/screenshot_") + timeBuf + "_" + std::to_string(counter++) + ".png";
				}

				if (savePngImage(filepath, fbW, fbH, pixels.data())) {
					printf("Saved screenshot to %s\n", filepath.c_str());
					if (g_app) {
						((MAIN_CLASS*)g_app)->gui.addMessage("Saved screenshot as " + filename);
					}
				} else {
					printf("Failed to save screenshot to %s\n", filepath.c_str());
				}
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		glfwSwapInterval(((MAIN_CLASS*)app)->options.vsync ? 1 : 0);
		if(((MAIN_CLASS*)app)->options.limitFramerate) {
			auto frameEnd = clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
			auto target = std::chrono::microseconds(33333); // ~30 fps
			if(elapsed < target)
				std::this_thread::sleep_for(target - elapsed);
		}
	}

	delete app;

	appContext.platform->finish();
	
	delete appContext.platform;
	
#ifndef STANDALONE_SERVER
	// Exit.
	glfwDestroyWindow(window);
	glfwTerminate();
#endif

	return 0;
}

#endif /*MAIN_GLFW_H__*/
