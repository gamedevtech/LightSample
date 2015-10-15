// ImGui SDL2 binding with OpenGL
// You can copy and use unmodified imgui_impl_* files in your project.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_ImplGLUT_Init();
IMGUI_API void        ImGui_ImplGLUT_NewFrame(int w, int h, double dTime);
IMGUI_API void        ImGui_ImplGLUT_Shutdown();