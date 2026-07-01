#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/SkinTextureCache.h"
#include "ui/panels/SkinChangerPanel.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

// ── OpenGL-based texture cache that loads real CS2 skin images from disk ────
class TestTextureCache final : public SkinTextureCache {
public:
    TestTextureCache() {
        icon_dir_ = "/tmp/skin_icons_padded/";
    }

    ~TestTextureCache() {
        for (auto& [id, tex] : textures_)
            glDeleteTextures(1, &tex);
        textures_.clear();
    }

    void ProcessPending() noexcept override {
        if (pending_.empty()) return;
        i32 pid = pending_.back();
        pending_.pop_back();
        LoadTexture(pid);
    }

    ImTextureID Get(i32 paint_id) noexcept override {
        auto it = textures_.find(paint_id);
        if (it != textures_.end())
            return (ImTextureID)(intptr_t)it->second;
        return ImTextureID(0);
    }

    bool IsLoaded(i32 paint_id) noexcept override {
        return textures_.count(paint_id) > 0;
    }

    void Request(i32 paint_id, WeaponsEnum) noexcept override {
        if (textures_.count(paint_id) || pending_set_.count(paint_id))
            return;
        // Check if we have the raw file
        auto path = icon_dir_ + std::to_string(paint_id) + ".rgba";
        if (!std::filesystem::exists(path))
            return;
        pending_.push_back(paint_id);
        pending_set_.insert(paint_id);
    }

    void InitMapping() noexcept override {}

private:
    std::unordered_map<i32, GLuint> textures_;
    std::vector<i32> pending_;
    std::unordered_set<i32> pending_set_;
    std::string icon_dir_;

    void LoadTexture(i32 paint_id) {
        auto path = icon_dir_ + std::to_string(paint_id) + ".rgba";
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return;

        // All padded skins are 1152x384 RGBA (3:1 aspect ratio)
        int w = 1152, h = 384;
        std::vector<unsigned char> pixels(w * h * 4);
        size_t read = fread(pixels.data(), 1, w * h * 4, f);
        fclose(f);
        if (read != (size_t)(w * h * 4)) return;

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        textures_[paint_id] = tex;
        fprintf(stderr, "Loaded texture %u for paint_id %d (%dx%d)\n", tex, paint_id, w, h);
    }
};

// ── Main ─────────────────────────────────────────────────────────────────────
int main() {
    if (!glfwInit()) return 1;

    GLFWwindow* win = glfwCreateWindow(820, 560, "VeneeR", nullptr, nullptr);
    if (!win) return 1;

    glfwMakeContextCurrent(win);
    if (gl3wInit()) return 1;

    fprintf(stderr, "GL: %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui_ImplGlfw_InitForOpenGL(win, false);
    ImGui_ImplOpenGL3_Init("#version 130");

    io.Fonts->Build();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    // ── Mock application state ────────────────────────────────────────────
    SkinDatabase db;
    SkinManager  mgr;
    SkinChanger  changer;
    SkinChangerPanel panel(db, mgr, changer);
    TestTextureCache tex_cache;
    panel.SetTextureCache(&tex_cache);

    mgr.SetSkin({180, 0.12f, 333, 42, "", true, "Fire Serpent", WeaponsEnum::Ak47});
    mgr.SetSkin({344, 0.18f, 777, 1337, "", true, "Dragon Lore", WeaponsEnum::Awp});
    panel.SetEquippedWeapon("AK-47 | Fire Serpent", 180);
    panel.SetOpen(true);

    mgr.SetTeamKnife(WeaponsEnum::Karambit, 420, true);
    mgr.SetTeamKnife(WeaponsEnum::Butterfly, 38, false);

    SkinInfo_t ct_glove{10042, 0.15f, 0, -1, "", false, "Imperial Plaid", WeaponsEnum::None};
    mgr.SetTeamGloveSkin(ct_glove, GlovesEnum::Driver, true);

    SkinInfo_t t_glove{10037, 0.20f, 0, -1, "", false, "Pandora's Box", WeaponsEnum::None};
    mgr.SetTeamGloveSkin(t_glove, GlovesEnum::HandWraps, false);

    // ── Render loop ───────────────────────────────────────────────────────
    for (int frame = 0; frame < 45; ++frame) {
        glfwPollEvents();

        // Keep processing pending textures until the cache is full
        for (int i = 0; i < 5; ++i)
            tex_cache.ProcessPending();

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

        if (frame == 40) {
            std::vector<unsigned char> pixels(820 * 560 * 3);
            glReadPixels(0, 0, 820, 560, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

            int non_bg = 0;
            for (size_t i = 0; i < pixels.size(); i += 3)
                if (pixels[i] > 25 || pixels[i+1] > 25 || pixels[i+2] > 35)
                    non_bg++;
            fprintf(stderr, "Non-bg: %d/%d (%.1f%%)\n",
                    non_bg, 820*560, 100.0*non_bg/(820*560));

            FILE* f = fopen("/tmp/veneer-screenshot.ppm", "wb");
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", 820, 560);
                for (int y = 559; y >= 0; --y)
                    fwrite(pixels.data() + y * 820 * 3, 1, 820 * 3, f);
                fclose(f);
                fprintf(stderr, "Screenshot saved\n");
            }
        }

        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    fprintf(stderr, "Done\n");
    return 0;
}
