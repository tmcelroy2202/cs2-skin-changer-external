#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "ui/panels/SkinChangerPanel.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* win = glfwCreateWindow(820, 560, "VeneeR", nullptr, nullptr);
    if (!win) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(win);
    if (gl3wInit()) {
        fprintf(stderr, "gl3wInit failed\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui_ImplGlfw_InitForOpenGL(win, false);
    ImGui_ImplOpenGL3_Init("#version 130");

    // ── Mock application state ────────────────────────────────────────────
    SkinDatabase db;
    SkinManager  mgr;
    SkinChanger  changer;
    SkinChangerPanel panel(db, mgr, changer);

    // Pre-select AK-47 | Fire Serpent (paint 180) for a nice default view
    mgr.SetSkin({180, 0.12f, 333, 42, "", true, "Fire Serpent", WeaponsEnum::Ak47});
    mgr.SetSkin({344, 0.18f, 777, 1337, "", true, "Dragon Lore", WeaponsEnum::Awp});
    panel.SetEquippedWeapon("AK-47 | Fire Serpent", 180);
    panel.SetOpen(true);

    // Pre-select some knife and glove for display
    mgr.SetTeamKnife(WeaponsEnum::Karambit, 420, true); // CT: Karambit Doppler Sapphire
    mgr.SetTeamKnife(WeaponsEnum::Butterfly, 38, false); // T: Butterfly Fade

    SkinInfo_t ct_glove{10042, 0.15f, 0, -1, "", false, "Imperial Plaid", WeaponsEnum::None};
    mgr.SetTeamGloveSkin(ct_glove, GlovesEnum::Driver, true);

    SkinInfo_t t_glove{10037, 0.20f, 0, -1, "", false, "Pandora's Box", WeaponsEnum::None};
    mgr.SetTeamGloveSkin(t_glove, GlovesEnum::HandWraps, false);

    // ── Render loop ───────────────────────────────────────────────────────
    for (int frame = 0; frame < 5; ++frame) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(820, 560), ImGuiCond_Always);
        panel.Draw();

        ImGui::Render();
        int fb_w, fb_h;
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.06f, 0.06f, 0.10f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);

        // Save screenshot on last frame
        if (frame == 4) {
            int w = 820, h = 560;
            std::vector<unsigned char> pixels(w * h * 3);
            glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            FILE* f = fopen("/tmp/veneer-screenshot.ppm", "wb");
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", w, h);
                // Flip vertically
                for (int y = h - 1; y >= 0; --y)
                    fwrite(pixels.data() + y * w * 3, 1, w * 3, f);
                fclose(f);
                fprintf(stderr, "Screenshot saved to /tmp/veneer-screenshot.ppm\n");
            }
        }
    }

    // ── Cleanup ────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();

    fprintf(stderr, "veneer-test: UI rendered successfully (3 frames)\n");
    return 0;
}
