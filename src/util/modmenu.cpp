#include "modmenu.h"
#include "config.h"

#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <mutex>
#include <cstring>

#include "pl/Gloss.h"

#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "ImGui/backends/imgui_impl_android.h"

#define TAG "[ShulkerPreview]"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool  g_initialized   = false;
static int   g_width = 0, g_height = 0;
static EGLContext g_targetcontext = EGL_NO_CONTEXT;
static EGLSurface g_targetsurface = EGL_NO_SURFACE;
static EGLBoolean (*orig_eglswapbuffers)(EGLDisplay, EGLSurface) = nullptr;

struct WindowBounds { float x, y, w, h; bool visible; };
static WindowBounds g_menuBounds = {0,0,0,0,false};
static std::mutex   g_boundsMutex;

static void drawmenu() {
    static bool show_menu = false;

    ImGuiIO& io = ImGui::GetIO();

    {
        const float btn_w = 220.0f, btn_h = 70.0f;
        const float btn_y = io.DisplaySize.y * 0.35f;
        ImGui::SetNextWindowPos(ImVec2(0.0f, btn_y), ImGuiCond_Always, ImVec2(0.0f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(btn_w, btn_h));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("SPTrigger", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);

        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2 bmin      = ImGui::GetWindowPos();
        ImVec2 bmax      = ImVec2(bmin.x + btn_w, bmin.y + btn_h);
        ImGui::InvisibleButton("##sp_trig", ImVec2(btn_w, btn_h));
        bool hovered     = ImGui::IsItemHovered();
        bool clicked     = ImGui::IsItemClicked();

        ImU32 bg = hovered ? IM_COL32(40,40,42,235) : IM_COL32(18,18,20,220);
        dl->AddRectFilled(bmin, bmax, bg, 7.0f);
        dl->AddRect      (bmin, bmax, IM_COL32(100,100,108,220), 7.0f, 0, 1.0f);

        const char* lbl = "Shulker Preview";
        ImFont* fnt = ImGui::GetFont();
        float fsz = 26.0f;
        ImVec2 lsz = fnt->CalcTextSizeA(fsz, FLT_MAX, -1.0f, lbl);
        dl->AddText(fnt, fsz,
            ImVec2(bmin.x + (btn_w - lsz.x)*0.5f, bmin.y + (btn_h - lsz.y)*0.5f),
            IM_COL32(235,235,235,255), lbl);

        if (clicked) show_menu = !show_menu;
        ImGui::End();

        {
            std::lock_guard<std::mutex> lk(g_boundsMutex);
            if (!show_menu)
                g_menuBounds = {0.0f, btn_y - btn_h*0.5f, btn_w, btn_h, true};
        }
    }

    if (!show_menu) return;

    ImGuiIO& iom = ImGui::GetIO();
    const float WIN_W = 520.0f, WIN_H = 340.0f;
    ImGui::SetNextWindowSize(ImVec2(WIN_W, WIN_H), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(iom.DisplaySize.x*0.5f, iom.DisplaySize.y*0.5f),
        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f,0.08f,0.10f,0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.28f,0.28f,0.35f,1.0f));

    ImGui::Begin("SP_Main", &show_menu,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 wpos  = ImGui::GetWindowPos();
    ImVec2 wsz   = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // rainbow header stripe
    float t = (float)ImGui::GetTime();
    float cr,cg,cb,cr2,cg2,cb2;
    ImGui::ColorConvertHSVtoRGB(fmodf(t*0.4f,1.0f),       1,1, cr, cg, cb);
    ImGui::ColorConvertHSVtoRGB(fmodf(t*0.4f+0.33f,1.0f), 1,1, cr2,cg2,cb2);
    dl->AddRectFilledMultiColor(wpos, ImVec2(wpos.x+wsz.x, wpos.y+5.0f),
        ImColor(cr,cg,cb), ImColor(cr2,cg2,cb2),
        ImColor(cr2,cg2,cb2), ImColor(cr,cg,cb));

    // title
    const float HDR_H = 56.0f;
    const char* ttl = "Shulker Preview";
    ImGui::SetWindowFontScale(1.4f);
    ImVec2 tsz = ImGui::CalcTextSize(ttl);
    ImGui::SetCursorPos(ImVec2((wsz.x - tsz.x)*0.5f, (HDR_H - tsz.y)*0.5f + 5.0f));
    ImGui::TextColored(ImVec4(0.95f,0.95f,0.95f,1.0f), "%s", ttl);
    ImGui::SetWindowFontScale(1.0f);

    const float CB_W = 44, CB_H = 28;
    ImVec2 cb_min = ImVec2(wpos.x+wsz.x-CB_W-10, wpos.y+(HDR_H-CB_H)*0.5f+5);
    ImVec2 cb_max = ImVec2(cb_min.x+CB_W, cb_min.y+CB_H);
    bool cb_hov = ImGui::IsMouseHoveringRect(cb_min, cb_max);
    dl->AddRectFilled(cb_min, cb_max, cb_hov?IM_COL32(220,40,40,255):IM_COL32(160,30,30,230), 5.0f);
    ImVec2 xsz = ImGui::CalcTextSize("X");
    dl->AddText(ImVec2(cb_min.x+(CB_W-xsz.x)*0.5f, cb_min.y+(CB_H-xsz.y)*0.5f),
        IM_COL32(255,255,255,255), "X");
    if (cb_hov && ImGui::IsMouseClicked(0)) show_menu = false;

    dl->AddLine(ImVec2(wpos.x+15, wpos.y+HDR_H+4), ImVec2(wpos.x+wsz.x-15, wpos.y+HDR_H+4),
        ImColor(0.20f,0.20f,0.26f,1.0f), 1.0f);

    const ImVec4 accent    = ImVec4(0.30f,0.67f,1.00f,1.0f);
    const ImVec4 text_main = ImVec4(0.92f,0.92f,0.92f,1.0f);
    const ImVec4 text_dim  = ImVec4(0.60f,0.60f,0.65f,1.0f);

    ImGui::PushStyleColor(ImGuiCol_CheckMark,        accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4(0.14f,0.14f,0.18f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   ImVec4(0.20f,0.20f,0.26f,1));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.50f,0.80f,1.00f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,             text_main);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(6,6));

    ImGui::SetCursorPos(ImVec2(30, HDR_H + 18));

    extern bool spKeyDown;
    ImGui::TextColored(text_dim, "Preview (tap to toggle)");
    ImGui::SetCursorPosX(30);
    if (ImGui::Button(spKeyDown ? "Preview: ON " : "Preview: OFF", ImVec2(wsz.x - 60, 0))) {
        spKeyDown = !spKeyDown;
    }

    ImGui::SetCursorPosX(30);
    ImGui::Dummy(ImVec2(0, 14));

    ImGui::SetCursorPosX(30);
    ImGui::TextColored(text_dim, "Tint Intensity");
    ImGui::SetCursorPosX(30);
    ImGui::PushItemWidth(wsz.x - 60);
    int tintPct = SP_clampPercent(static_cast<int>(spTintIntensity * 100.0f + 0.5f));
    if (ImGui::SliderInt("##tint", &tintPct, 0, 200, "%d%%")) {
        spTintIntensity = static_cast<float>(tintPct) / 100.0f;
        SP_saveConfig();
    }
    ImGui::PopItemWidth();

    ImGui::SetCursorPosX(30);
    ImGui::Dummy(ImVec2(0, 8));

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);

    {
        std::lock_guard<std::mutex> lk(g_boundsMutex);
        g_menuBounds = {wpos.x, wpos.y, wsz.x, wsz.y, true};
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

static void sp_setup() {
    if (g_initialized || g_width <= 0) return;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImFontConfig fcfg;
    fcfg.OversampleH = 3; fcfg.OversampleV = 2; fcfg.PixelSnapH = true;
    const char* font_paths[] = {
        "/system/fonts/Roboto-Medium.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/NotoSans-Regular.ttf",
        "/system/fonts/DroidSans.ttf"
    };
    bool ok = false;
    for (auto* p : font_paths)
        if (io.Fonts->AddFontFromFileTTF(p, 26.0f, &fcfg)) { ok = true; break; }
    if (!ok) { io.Fonts->AddFontDefault(); io.FontGlobalScale = 1.3f; }

    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");
    g_initialized = true;
}

static void sp_render() {
    if (!g_initialized) return;

    GLint last_prog, last_ab, last_eab, last_fbo, last_vp[4];
    GLint last_tex0, last_tex1, last_at;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &last_at);
    glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_tex0);
    glActiveTexture(GL_TEXTURE1); glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_tex1);
    glActiveTexture(last_at);
    glGetIntegerv(GL_CURRENT_PROGRAM,              &last_prog);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,         &last_ab);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_eab);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,          &last_fbo);
    glGetIntegerv(GL_VIEWPORT,                     last_vp);
    GLboolean last_scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean last_depth   = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_blend   = glIsEnabled(GL_BLEND);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_width, (float)g_height);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(g_width, g_height);
    ImGui::NewFrame();
    drawmenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glUseProgram(last_prog);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, last_tex0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, last_tex1);
    glActiveTexture(last_at);
    glBindBuffer(GL_ARRAY_BUFFER, last_ab);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_eab);
    glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
    glViewport(last_vp[0], last_vp[1], last_vp[2], last_vp[3]);
    if (last_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (last_depth)   glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
    if (last_blend)   glEnable(GL_BLEND);         else glDisable(GL_BLEND);
}

static EGLBoolean hook_eglswapbuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!orig_eglswapbuffers) return EGL_FALSE;
    EGLContext ctx = eglGetCurrentContext();
    if (ctx == EGL_NO_CONTEXT ||
        (g_targetcontext != EGL_NO_CONTEXT && (ctx != g_targetcontext || surf != g_targetsurface)))
        return orig_eglswapbuffers(dpy, surf);

    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH,  &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w < 100 || h < 100) return orig_eglswapbuffers(dpy, surf);

    if (g_targetcontext == EGL_NO_CONTEXT) { g_targetcontext = ctx; g_targetsurface = surf; }
    g_width = w; g_height = h;

    sp_setup();
    sp_render();

    return orig_eglswapbuffers(dpy, surf);
}

typedef bool (*PreloaderInput_OnTouch_Fn)(int action, int pointerId, float x, float y);
struct PreloaderInput_Interface {
    void (*RegisterTouchCallback)(PreloaderInput_OnTouch_Fn);
};
typedef PreloaderInput_Interface* (*GetPreloaderInput_Fn)();

static bool OnTouchCallback(int action, int pointerId, float x, float y) {
    (void)pointerId;
    if (!g_initialized) return false;

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    if      (action == AMOTION_EVENT_ACTION_DOWN) io.AddMouseButtonEvent(0, true);
    else if (action == AMOTION_EVENT_ACTION_UP)   io.AddMouseButtonEvent(0, false);

    bool hit = false;
    {
        std::lock_guard<std::mutex> lk(g_boundsMutex);
        if (g_menuBounds.visible &&
            x >= g_menuBounds.x && x <= g_menuBounds.x + g_menuBounds.w &&
            y >= g_menuBounds.y && y <= g_menuBounds.y + g_menuBounds.h)
            hit = true;
    }
    return hit || io.WantCaptureMouse;
}

static void* imgui_thread(void*) {
    sleep(3); // wait for game libs to load
    GlossInit(true);
    GHandle hegl = GlossOpen("libEGL.so");
    if (hegl) {
        void* swap = (void*)GlossSymbol(hegl, "eglSwapBuffers", nullptr);
        if (swap) GlossHook(swap, (void*)hook_eglswapbuffers, (void**)&orig_eglswapbuffers);
    }

    void* preloaderLib = dlopen("libpreloader.so", RTLD_NOW);
    if (preloaderLib) {
        auto GetInput = (GetPreloaderInput_Fn)dlsym(preloaderLib, "GetPreloaderInput");
        if (GetInput) {
            PreloaderInput_Interface* inp = GetInput();
            if (inp && inp->RegisterTouchCallback)
                inp->RegisterTouchCallback(OnTouchCallback);
        }
    }
    return nullptr;
}

void SP_initModMenu() {
    pthread_t t;
    pthread_create(&t, nullptr, imgui_thread, nullptr);
    pthread_detach(t);
    LOGI("ImGui menu thread started");
}
