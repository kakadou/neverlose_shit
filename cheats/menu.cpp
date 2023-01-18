#include "menu.h"
#include "../constchars.h"
#include "misc/logs.h"
#include <shlobj_core.h>

std::string preview = "";

void draw_multicombo(std::string name, std::vector<int>& variable, const char* labels[], int count, std::string& preview)
{
    auto hashname = name; // we dont want to render name of combo

    for (auto i = 0, j = 0; i < count; i++)
    {
        if (variable[i])
        {
            if (j)
                preview += crypt_str(", ") + (std::string)labels[i];
            else
                preview = labels[i];

            j++;
        }
    }

    if (ImGui::BeginCombo(hashname.c_str(), preview.c_str()))
    {
        ImGui::Spacing();
        ImGui::BeginGroup();
        {

            for (auto i = 0; i < count; i++)
                ImGui::Selectable(labels[i], (bool*)&variable[i], ImGuiSelectableFlags_DontClosePopups);

        }
        ImGui::EndGroup();
        ImGui::Spacing();

        ImGui::EndCombo();
    }

    preview = crypt_str("Select");
}

std::string get_config_dir()
{
    std::string folder;
    static TCHAR path[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(NULL, 0x001a, NULL, NULL, path)))
        folder = std::string(path) + crypt_str("\\neverlose.cc\\Configs\\");

    CreateDirectory(folder.c_str(), NULL);
    return folder;
}

std::vector<std::string> scripts;
std::vector<std::string> files;

int selected_script = 0;
int selected_config = 0;

void load_config()
{
    if (cfg_manager->files.empty())
        return;

    cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), false);
    c_lua::get().unload_all_scripts();

    for (auto& script : g_cfg.scripts.scripts)
        c_lua::get().load_script(c_lua::get().get_script_id(script));

    scripts = c_lua::get().scripts;

    if (selected_script >= scripts.size())
        selected_script = scripts.size() - 1; //-V103

    for (auto& current : scripts)
    {
        if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
            current.erase(current.size() - 5, 5);
        else if (current.size() >= 4)
            current.erase(current.size() - 4, 4);
    }

    g_cfg.scripts.scripts.clear();

    cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), true);
    cfg_manager->config_files();

    eventlogs::get().add(crypt_str("Loaded ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void save_config()
{
    if (cfg_manager->files.empty())
        return;

    g_cfg.scripts.scripts.clear();

    for (auto i = 0; i < c_lua::get().scripts.size(); ++i)
    {
        auto script = c_lua::get().scripts.at(i);

        if (c_lua::get().loaded.at(i))
            g_cfg.scripts.scripts.emplace_back(script);
    }

    cfg_manager->save(cfg_manager->files.at(g_cfg.selected_config));
    cfg_manager->config_files();

    eventlogs::get().add(crypt_str("Saved ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void remove_config()
{
    if (cfg_manager->files.empty())
        return;

    eventlogs::get().add(crypt_str("Removed ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);

    cfg_manager->remove(cfg_manager->files.at(g_cfg.selected_config));
    cfg_manager->config_files();

    files = cfg_manager->files;

    if (g_cfg.selected_config >= files.size())
        g_cfg.selected_config = files.size() - 1; //-V103

    for (auto& current : files)
        if (current.size() > 2)
            current.erase(current.size() - 3, 3);
}

void add_config()
{
    auto empty = true;

    for (auto current : g_cfg.new_config_name)
    {
        if (current != ' ')
        {
            empty = false;
            break;
        }
    }

    if (empty)
        g_cfg.new_config_name = crypt_str("config");

    eventlogs::get().add(crypt_str("Added ") + g_cfg.new_config_name + crypt_str(" config"), false);

    if (g_cfg.new_config_name.find(crypt_str(".ob")) == std::string::npos)
        g_cfg.new_config_name += crypt_str(".ob");

    cfg_manager->save(g_cfg.new_config_name);
    cfg_manager->config_files();

    g_cfg.selected_config = cfg_manager->files.size() - 1; //-V103
    files = cfg_manager->files;

    for (auto& current : files)
        if (current.size() > 2)
            current.erase(current.size() - 3, 3);
}

void c_menu::draw_info_window(bool open, int* scale, int* color, float* speed) 
{
    static float alpha = 0.f;
    alpha = ImLerp(alpha, open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 8.f * *speed);
    if (alpha < 0.1f)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(305 * ImGui::GetIO().GUIScale, 450 * ImGui::GetIO().GUIScale));
    ImGui::Begin("Info window", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    {
        ImVec2 cursor_pos = ImGui::GetCursorPos();

        ImGui::SetCursorPos(ImVec2(0, 120 * ImGui::GetIO().GUIScale));
        ImGui::BeginGroup();
        {
            ImGui::SetCursorPosX(0);
            ImGui::SetCursorPosY(290 * ImGui::GetIO().GUIScale);
            static bool bred_ebaniy = false;
            ImGui::Checkbox("Auto Save", &bred_ebaniy);
            ImGui::Combo("Dpi Scale", scale, "50%\0" "75%\0" "100%\0" "125%\0" "150%\0" "175%\0" "200%\0");
            ImGui::SliderFloat("Animation Speed", speed, 0.1f, 2.f, "%.1f");
            ImGui::SetCursorPosX(10); ImGui::TextColored(Colors::Text, "Style");
            ImGui::SameLine(210 * ImGui::GetIO().GUIScale); if (ImGui::RadioButton("Blue", *color == 0, 0)) *color = 0;
            ImGui::SameLine(); if (ImGui::RadioButton("White", *color == 2, 1)) *color = 2;
            ImGui::SameLine(); if (ImGui::RadioButton("Black", *color == 1, 2)) *color = 1;

            ImGui::Spacing(); ImGui::SetCursorPos(ImVec2(25, 120 * ImGui::GetIO().GUIScale));
            ImGui::TextColored(Colors::TextActive, "Version:"); ImGui::SameLine(); ImGui::TextColored(Colors::Accent, "3.0");
            ImGui::Spacing(); ImGui::SetCursorPosX(25);
            ImGui::TextColored(Colors::TextActive, "Build date:"); ImGui::SameLine(); ImGui::TextColored(Colors::Accent, "Jan 18 2023");
            ImGui::Spacing(); ImGui::SetCursorPosX(25);
            ImGui::TextColored(Colors::TextActive, "Build type:"); ImGui::SameLine(); ImGui::TextColored(Colors::Accent, "Dev");
            ImGui::Spacing(); ImGui::SetCursorPosX(25);
            ImGui::TextColored(Colors::TextActive, "Registered to:"); ImGui::SameLine(); ImGui::TextColored(Colors::Accent, g_ctx.username.c_str());
            ImGui::Spacing(); ImGui::SetCursorPosX(25);
            ImGui::TextColored(Colors::TextActive, "Subscription till:"); ImGui::SameLine(); ImGui::TextColored(Colors::Accent, "31.12.2029 23:59");
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize("ezcrack.cc - 2023").x / 2);
            ImGui::TextColored(Colors::TextActive, "neverlose.cc - 2023");
        }
        ImGui::EndGroup();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

bool LabelClick(const char* label, bool* v, const char* unique_id)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    // The concatoff/on thingies were for my weapon config system so if we're going to make that, we still need this aids.
    char Buf[64];
    _snprintf(Buf, 62, crypt_str("%s"), label);

    char getid[128];
    sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(getid);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
    ImGui::ItemSize(check_bb, style.FramePadding.y);

    ImRect total_bb = check_bb;

    if (label_size.x > 0)
    {
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

        ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
        total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
    }

    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        *v = !(*v);

    if (*v)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(126 / 255.f, 131 / 255.f, 219 / 255.f, 1.f));
    if (label_size.x > 0.0f)
        ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
    if (*v)
        ImGui::PopStyleColor();

    return pressed;

}

void draw_keybind(const char* label, key_bind* key_bind, const char* unique_id)
{
    // reset bind if we re pressing esc
    if (key_bind->key == KEY_ESCAPE)
        key_bind->key = KEY_NONE;

    auto clicked = false;
    auto text = (std::string)m_inputsys()->ButtonCodeToString(key_bind->key);
    for (int i = 0; i < strlen(m_inputsys()->ButtonCodeToString(key_bind->key)); i++) {
        text[i] = tolower(m_inputsys()->ButtonCodeToString(key_bind->key)[i]);
    }
    if (text == "LEFTARROW")
        text = "<";
    if (text == "RIGHTARROW")
        text = "<";
    if (text == "UPARROW")
        text = "^";
    if (text == "DOWNARROW")
        text = "↓";
    if (key_bind->key <= KEY_NONE || key_bind->key >= KEY_MAX)
        text = "none";

    // if we clicked on keybind
    if (hooks::input_shouldListen && hooks::input_receivedKeyval == &key_bind->key)
    {
        clicked = true;
        text = "...";
    }

    if (ImGui::KeybindButton(label, unique_id, ImVec2(0, 0), clicked, 0, text.c_str()))
        clicked = true;


    if (clicked)
    {
        hooks::input_shouldListen = true;
        hooks::input_receivedKeyval = &key_bind->key;
    }

    static auto hold = false, toggle = false;

    switch (key_bind->mode)
    {
    case HOLD:
        hold = true;
        toggle = false;
        break;
    case TOGGLE:
        toggle = true;
        hold = false;
        break;
    }

    if (ImGui::BeginPopup(unique_id))
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6 * c_menu::get().dpi_scale);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Hold")).x / 2)));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 19);

        if (LabelClick(crypt_str("Hold"), &hold, unique_id))
        {
            if (hold)
            {
                toggle = false;
                key_bind->mode = HOLD;
            }
            else if (toggle)
            {
                hold = false;
                key_bind->mode = TOGGLE;
            }
            else
            {
                toggle = false;
                key_bind->mode = HOLD;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Toggle")).x / 2)));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 19);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9 * c_menu::get().dpi_scale);

        if (LabelClick(crypt_str("Toggle"), &toggle, unique_id))
        {
            if (toggle)
            {
                hold = false;
                key_bind->mode = TOGGLE;
            }
            else if (hold)
            {
                toggle = false;
                key_bind->mode = HOLD;
            }
            else
            {
                hold = false;
                key_bind->mode = TOGGLE;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void c_menu::draw(bool is_open)
{
    auto& io = ImGui::GetIO();

    static float dpi_scale = 1.f; static int dpi_set = 2;
    io.DisplayFramebufferScale = ImVec2(dpi_scale, dpi_scale);
    io.DisplaySize.x /= dpi_scale;
    io.DisplaySize.y /= dpi_scale;

    switch (dpi_set) {
    case 0: dpi_scale = 0.70f; break;
    case 1: dpi_scale = 0.85f; break;
    case 2: dpi_scale = 1.f; break;
    case 3: dpi_scale = 1.15f; break;
    case 4: dpi_scale = 1.30f; break;
    case 5: dpi_scale = 1.45f; break;
    case 6: dpi_scale = 1.60f; break;
    }
    io.GUIScale = dpi_scale;
    if (io.MousePos.x != -FLT_MAX && io.MousePos.y != -FLT_MAX)
    {
        io.MousePos.x /= dpi_scale;
        io.MousePos.y /= dpi_scale;
    }

    ImGui::NewFrame();

    static int theme = 0;

    Colors::LeftBar = ImLerp(Colors::LeftBar, theme == 1 ? ImColor(8, 8, 8, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha * 0.75f)).Value : ImColor(3, 30, 50, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Background = ImLerp(Colors::Background, theme == 1 ? ImColor(8, 8, 8, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(9, 8, 13, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Child = ImLerp(Colors::Child, theme == 1 ? ImColor(11, 11, 13, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(250, 250, 250, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(1, 11, 21, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::ChildText = ImLerp(Colors::ChildText, theme == 1 ? ImColor(221, 220, 225, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(0, 0, 0, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(200, 203, 208, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Borders = ImLerp(Colors::Borders, theme == 1 ? ImColor(26, 26, 26, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(217, 217, 217, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(12, 33, 52, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::BordersActive = ImLerp(Colors::BordersActive, theme == 1 ? ImColor(36, 34, 38, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(217, 217, 217, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(4, 25, 40, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::TabFrame = ImLerp(Colors::TabFrame, theme == 1 ? ImColor(62, 63, 58, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(214, 213, 213, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(1, 72, 104, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::TabText = ImLerp(Colors::TabText, theme == 1 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(38, 38, 38, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Logo = ImLerp(Colors::Logo, theme == 1 ? ImColor(253, 254, 255, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(51, 51, 51, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(248, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Text = ImLerp(Colors::Text, theme == 1 ? ImColor(162, 176, 185, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(77, 77, 77, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(162, 176, 185, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::TextActive = ImLerp(Colors::TextActive, theme == 1 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(6, 6, 6, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::Accent = ImLerp(Colors::Accent, theme == 2 ? ImColor(0, 165, 243, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(0, 165, 243, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 6.f);
    Colors::AccentDisabled = ImLerp(Colors::AccentDisabled, theme == 1 ? ImColor(76, 90, 101, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(76, 90, 101, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);

    Colors::Frame = ImLerp(Colors::Frame, theme == 1 ? ImColor(3, 6, 15, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(231, 231, 231, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(2, 5, 12, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::FrameDisabled = ImLerp(Colors::FrameDisabled, theme == 1 ? ImColor(8, 8, 8, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(8, 9, 13, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::FrameHovered = ImLerp(Colors::FrameHovered, theme == 1 ? ImColor(13, 13, 13, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(255, 255, 255, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(2, 5, 12, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::FrameActive = ImLerp(Colors::FrameActive, theme == 1 ? ImColor(4, 25, 46, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(0, 119, 188, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(2, 18, 33, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::FrameOpened = ImLerp(Colors::FrameOpened, theme == 1 ? ImColor(32, 31, 33, int(255 * ImGui::GetStyle().Alpha)).Value : theme == 2 ? ImColor(238, 238, 238, int(255 * ImGui::GetStyle().Alpha)).Value : ImColor(2, 18, 33, int(255 * ImGui::GetStyle().Alpha)).Value, io.DeltaTime * 12.f * io.AnimationSpeed);
    Colors::theme = theme;
    Colors::username = g_ctx.username;

    menu_style = theme;

    static int ActiveTab = 0;
    static int RageWeapon = 0;

    const char* rage_weapons[8] = { crypt_str("Heavy Pistols"), crypt_str("Pistols"), crypt_str("SMG"), crypt_str("Rifles"), crypt_str("Auto"), crypt_str("Scout"), crypt_str("AWP"), crypt_str("Heavy") };
    
    if (is_open)
    {
        ImGui::SetNextWindowSize(ImVec2(800 * io.GUIScale, 620 * io.GUIScale));
        ImGui::MainBegin("Menu", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImGui::SetCursorPos(ImVec2(0.f, 62.f * io.GUIScale));
            ImGui::BeginGroup( /* Tabs place */);
            {
                
                ImGui::SetCursorPosX(25); ImGui::TextColored(Colors::Text, "Aimbot");
                if (ImGui::TabButton("A", "Ragebot", ActiveTab == 0)) ActiveTab = 0;
                if (ImGui::TabButton("B", "Anti Aim", ActiveTab == 1)) ActiveTab = 1;
                if (ImGui::TabButton("C", "Legitbot", ActiveTab == 2)) ActiveTab = 2;

                ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                ImGui::SetCursorPosX(25); ImGui::TextColored(Colors::Text, "Visuals");
                if (ImGui::TabButton("D", "Players", ActiveTab == 3)) ActiveTab = 3;
                if (ImGui::TabButton("E", "Weapon", ActiveTab == 4)) ActiveTab = 4;
                if (ImGui::TabButton("F", "Grenades", ActiveTab == 5)) ActiveTab = 5;
                if (ImGui::TabButton("G", "World", ActiveTab == 7)) ActiveTab = 7;
                if (ImGui::TabButton("H", "View", ActiveTab == 8)) ActiveTab = 8;

                ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

                ImGui::SetCursorPosX(25); ImGui::TextColored(Colors::Text, "Miscellaneous");
                if (ImGui::TabButton("I", "Main", ActiveTab == 9)) ActiveTab = 9;
                if (ImGui::TabButton("J", "Inventory", ActiveTab == 10)) ActiveTab = 10;
                if (ImGui::TabButton("K", "Scripts", ActiveTab == 11)) ActiveTab = 11;
                if (ImGui::TabButton("L", "Configs", ActiveTab == 12)) ActiveTab = 12;
            }
            ImGui::EndGroup();

            ImGui::SetCursorPos(ImVec2(200.f * io.GUIScale, 0));
            ImGui::BeginGroup( /* Info place */);
            {
                ImGui::SetCursorPosY(10 * io.GUIScale);
                ImGui::Button("N", "Save", ImVec2(100 * io.GUIScale, 30 * io.GUIScale));

                static bool opened = false;

                ImGui::SetCursorPos(ImVec2(800 - 70 * io.GUIScale, 10 * io.GUIScale));
                ImGui::PushFont(io.Fonts->Fonts[4]);
                if (ImGui::ButtonEx("L", ImVec2(25, 30), ImGuiButtonFlags_TextOnly)) opened = !opened;
                ImGui::SameLine();
                ImGui::ButtonEx("M", ImVec2(25, 30), ImGuiButtonFlags_TextOnly);
                ImGui::PopFont();

                draw_info_window(opened, &dpi_set, &theme, &io.AnimationSpeed);
            }
            ImGui::EndGroup();

            ImGui::SetCursorPos(ImVec2(180.f * io.GUIScale, 50.f * io.GUIScale));
            ImGui::BeginChild("content", ImVec2(1110, 1110));
            {
                int ChildW = 275 * io.GUIScale;
                if (ActiveTab == 0)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Main", ImVec2(ChildW, 200));
                    {
                        ImGui::Checkbox("Enable Ragebot", &g_cfg.ragebot.enable);
                        ImGui::Combo("Weapon", &RageWeapon, rage_weapons, ARRAYSIZE(rage_weapons));
                        ImGui::Checkbox("Silent Aim", &g_cfg.ragebot.silent_aim);
                        ImGui::SliderInt("FOV", &g_cfg.ragebot.field_of_view, 0, 180);
                        ImGui::Checkbox("Auto Wall", &g_cfg.ragebot.autowall);
                        ImGui::Checkbox("Auto Fire", &g_cfg.ragebot.autoshoot);
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Exploits", ImVec2(ChildW, 140));
                    {
                        const char* suka[] =
                        {
                            "Default\0",
                            "Fastest\0"
                        };

                        static int Combos[2];
                        ImGui::Checkbox("Hide Shots", &g_cfg.antiaim.hide_shots);
                        ImGui::Checkbox("Double Tap", &g_cfg.ragebot.double_tap);

                        if (g_cfg.ragebot.double_tap)
                        {
                            draw_keybind(crypt_str("Doubletap"), &g_cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DT"));
                        }
                        if (g_cfg.antiaim.hide_shots)
                        {
                            draw_keybind(crypt_str("Hideshots"), &g_cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"));
                        }
                        ImGui::Combo("Mode", &g_cfg.antiaim.antiaim_type, suka, ARRAYSIZE(suka));
                        ImGui::Checkbox("Teleport Boost", &g_cfg.ragebot.slow_teleport);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 287));

                    ImGui::MenuChild("Accuracy", ImVec2(ChildW, 263));
                    {
                        g_cfg.ragebot.weapon[RageWeapon].hitchance = true;
                        g_cfg.ragebot.weapon[RageWeapon].max_misses = true;

                        draw_multicombo("Hitboxes", g_cfg.ragebot.weapon[RageWeapon].hitboxes, hitboxes, ARRAYSIZE(hitboxes), preview);
                        ImGui::SliderInt("Hit Chance", &g_cfg.ragebot.weapon[RageWeapon].hitchance_amount, 0, 100);
                        ImGui::Checkbox("Static Point Scale", &g_cfg.ragebot.weapon[RageWeapon].static_point_scale);
                        ImGui::SliderFloat("Head Scale", &g_cfg.ragebot.weapon[RageWeapon].head_scale, 0.0, 1.0);
                        ImGui::SliderFloat("Body Scale", &g_cfg.ragebot.weapon[RageWeapon].body_scale, 0.0, 1.0);
                        ImGui::Checkbox("Prefer Safe Points", &g_cfg.ragebot.weapon[RageWeapon].prefer_safe_points);
                        draw_keybind(crypt_str("Force safe points"), &g_cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));
                        ImGui::Checkbox("Prefer Body Aim", &g_cfg.ragebot.weapon[RageWeapon].prefer_body_aim);
                        draw_keybind(crypt_str("Force body aim"), &g_cfg.ragebot.body_aim_key, crypt_str("##HOKEY_FORCE_BODY_AIM"));
                        ImGui::SliderInt("Max Misses", &g_cfg.ragebot.weapon[RageWeapon].max_misses_amount, 0, 5);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(ChildW + 16 + 18, 227));

                    ImGui::MenuChild("Min. Damage", ImVec2(ChildW, 80));
                    {
                        draw_keybind(crypt_str("Damage override"), &g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, crypt_str("##HOTKEY__DAMAGE_OVERRIDE"));

                        if (g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX)
                            ImGui::SliderInt(crypt_str("Minimum override damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_override_damage, 1, 120);

                        ImGui::SliderInt("Visible", &g_cfg.ragebot.weapon[RageWeapon].minimum_visible_damage, 0, 120);
                        ImGui::SliderInt("Autowall", &g_cfg.ragebot.weapon[RageWeapon].minimum_damage, 0, 120);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(ChildW + 16 + 18, 345));

                    ImGui::MenuChild("Misc", ImVec2(ChildW, 110));
                    {
                        ImGui::Checkbox("Auto Stop", &g_cfg.ragebot.weapon[RageWeapon].autostop);
                        draw_multicombo("Conditions", g_cfg.ragebot.weapon[RageWeapon].autostop_modifiers, autostop_modifiers, ARRAYSIZE(autostop_modifiers), preview);
                        ImGui::Checkbox("Auto Scope", &g_cfg.ragebot.autoscope);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 1)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Main", ImVec2(ChildW, 202));
                    {
                        ImGui::Checkbox("Enable Anti Aim", &g_cfg.antiaim.enable);
                        
                        ImGui::Combo("Pitch", &g_cfg.antiaim.pitch_type, PitchType, ARRAYSIZE(PitchType));
                        ImGui::Combo("Yaw Base", &g_cfg.antiaim.yaw_base, YawBase, ARRAYSIZE(YawBase));
                        ImGui::SliderInt("Yaw Add", &g_cfg.antiaim.yaw_offset, -180, 180);
                        ImGui::Combo("Yaw Modifier", &g_cfg.antiaim.yaw_modifier, YawModifier, ARRAYSIZE(YawModifier));
                        ImGui::SliderInt("Yaw Modifier Deg", &g_cfg.antiaim.yaw_jitter, 0, 180);
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);
                    ImGui::MenuChild("Fake Angle", ImVec2(ChildW, 264));
                    {
                        ImGui::Checkbox("Enable Fake Angle", &g_cfg.antiaim.fake_yaw);

                        ImGui::Checkbox("Inverter", &g_cfg.antiaim.inverter);
                        
                        draw_keybind(crypt_str("Invert desync"), &g_cfg.antiaim.flip_desync, crypt_str("##HOTKEY_INVERT_DESYNC"));
                        
                        ImGui::SliderInt("Left Limit", &g_cfg.antiaim.desync_angle, 0, 60);
                        ImGui::SliderInt("Right Limit", &g_cfg.antiaim.invert_desync_angle, 0, 60);
                        draw_multicombo("Fake Options", g_cfg.antiaim.desync_options, desync_options, ARRAYSIZE(desync_options), preview);
                        ImGui::Combo("LBY Mode", &g_cfg.antiaim.lby_mode, lby_type, ARRAYSIZE(lby_type));
                        ImGui::Combo("Freestanding", &g_cfg.antiaim.freestand_desync, FreestadingType, ARRAYSIZE(FreestadingType));
                        ImGui::Combo("Desync On Shot", &g_cfg.antiaim.desync_onshot, DesyncOnShot, ARRAYSIZE(DesyncOnShot));
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 294));
                    ImGui::MenuChild("Fake Lag", ImVec2(ChildW, 175));
                    {
                        ImGui::Checkbox("Enable Fake Lag", &g_cfg.antiaim.fakelag);

                        ImGui::Combo("Type", &g_cfg.antiaim.fakelag_type, fakelags, ARRAYSIZE(fakelags));
                        draw_multicombo("Triggers", g_cfg.antiaim.fakelag_enablers, lagstrigger, ARRAYSIZE(lagstrigger), preview);
                        ImGui::SliderInt("Trigger Limit", &g_cfg.antiaim.triggers_fakelag_amount, 0, 14);
                        ImGui::SliderInt("Limit", &g_cfg.antiaim.fakelag_amount, 0, 14);
                        ImGui::SliderInt("Randomization", &g_cfg.antiaim.yaw_type, 0, 100);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(ChildW + 18 + 16, 355));
                    ImGui::MenuChild("Misc", ImVec2(ChildW, 47));
                    {
                        ImGui::Checkbox("Slide Walk", &g_cfg.misc.slidewalk);
                        draw_keybind(crypt_str("Manual back"), &g_cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));

                        draw_keybind(crypt_str("Manual left"), &g_cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));

                        draw_keybind(crypt_str("Manual right"), &g_cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));

                       
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 3)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("ESP", ImVec2(ChildW, 266));
                    {
                        ImGui::Checkbox("Enable ESP", &g_cfg.player.enable);
                        ImGui::Checkbox("Box", &g_cfg.player.type[ENEMY].box);
                        ImGui::Checkbox("Name", &g_cfg.player.type[ENEMY].name);
                        ImGui::Checkbox("Health", &g_cfg.player.type[ENEMY].health);
                        ImGui::Checkbox("Skeleton", &g_cfg.player.type[ENEMY].skeleton);
                        ImGui::Checkbox("Ammo", &g_cfg.player.type[ENEMY].ammo);
                        ImGui::Checkbox("In-Game radar", &g_cfg.misc.ingame_radar);
                        draw_multicombo("Flags", g_cfg.player.type[ENEMY].flags, flags, ARRAYSIZE(flags), preview);
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Chams", ImVec2(ChildW, 80));
                    {
                        draw_multicombo(crypt_str("Chams Type"), g_cfg.player.type[ENEMY].chams, g_cfg.player.type[ENEMY].chams[PLAYER_CHAMS_VISIBLE] ? chamsvisact : chamsvis, g_cfg.player.type[ENEMY].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvisact) : ARRAYSIZE(chamsvis), preview);
                        ImGui::Combo("Player Material", &g_cfg.player.type[ENEMY].chams_type, chamstype, ARRAYSIZE(chamstype));
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 359));
                    ImGui::MenuChild("Glow", ImVec2(ChildW, 47));
                    {
                        ImGui::Checkbox("Enable Glow", &g_cfg.player.type[ENEMY].glow);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 4)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("ESP", ImVec2(ChildW, 48));
                    {
                        draw_multicombo("Weapon Projectiles", g_cfg.esp.weapon, weaponesp, ARRAYSIZE(weaponesp), preview);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 5)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("ESP", ImVec2(ChildW, 173));
                    {
                        ImGui::Checkbox("Prediction", &g_cfg.esp.grenade_prediction);

                        if (g_cfg.esp.grenade_prediction)
                            ImGui::Checkbox("On Click", &g_cfg.esp.on_click);

                        ImGui::Checkbox("Molotov Timer", &g_cfg.esp.molotov_timer);
                        ImGui::Checkbox("Molotov Wireframe", &g_cfg.esp.molotov_wireframe);
                        ImGui::Checkbox("Smoke Timer", &g_cfg.esp.smoke_timer);
                        draw_multicombo("Grenade Projectiles", g_cfg.esp.grenade_esp, proj_combo, ARRAYSIZE(proj_combo), preview);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 7)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Main", ImVec2(ChildW, 265));
                    {
                        ImGui::Checkbox("Night Mode", &g_cfg.esp.nightmode);
                        ImGui::Combo("SkyBox", &g_cfg.esp.skybox, skybox, ARRAYSIZE(skybox));
                        draw_multicombo("Removals", g_cfg.esp.removals, removals, ARRAYSIZE(removals), preview);
                        ImGui::Checkbox("Ragdoll Physics", &g_cfg.misc.ragdolls);

                        draw_multicombo("Hit Marker", g_cfg.esp.hitmarker, hitmarkers, ARRAYSIZE(hitmarkers), preview);
                        ImGui::Combo("Hit Sound", &g_cfg.esp.hitsound, sounds, ARRAYSIZE(sounds));
                        ImGui::Checkbox("Bomb Timer", &g_cfg.esp.bomb_timer);

                        ImGui::Checkbox("Damage Marker", &g_cfg.esp.damage_marker);
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Misc", ImVec2(ChildW, 110));
                    {
                        //ImGui::Combo("SkyBox", &g_cfg.esp.penetration_reticle, skybox, ARRAYSIZE(skybox));
                        ImGui::Checkbox("Autowall Crosshair", &g_cfg.esp.penetration_reticle);
                        ImGui::Checkbox("Damage Indicator", &g_cfg.esp.damage_marker);
                        ImGui::Checkbox("Preserve Kill Feed", &g_cfg.esp.preserve_killfeed);
                       // ImGui::Checkbox("Weather", &g_cfg.esp.rain);
                        //ImGui::Checkbox("Taser Range", &g_cfg.esp.taser_range);

                        //ImGui::Checkbox("World Modulation", &g_cfg.esp.world_modulation);
                        
                        //if (g_cfg.esp.world_modulation)
                        //{
                        //    ImGui::SliderFloat(crypt_str("Exposure"), &g_cfg.esp.exposure, 0.0f, 2000.0f);
                        //    ImGui::SliderFloat(crypt_str("Bloom"), &g_cfg.esp.bloom, 0.0f, 750.0f);
                        //    ImGui::SliderFloat(crypt_str("Ambient"), &g_cfg.esp.ambient, 0.0f, 1500.0f);
                        //}
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18 + ChildW + 16, 48 + 110 + 42));
                    ImGui::MenuChild("Hit", ImVec2(ChildW, 139));
                    {
                        draw_multicombo("Hit Marker", g_cfg.esp.hitmarker, hitmarkers, ARRAYSIZE(hitmarkers), preview);
                        ImGui::Combo("Hit Sound", &g_cfg.esp.hitsound, sounds, ARRAYSIZE(sounds));
                        ImGui::Checkbox("Client Bullet Impacts", &g_cfg.esp.client_bullet_impacts);
                        ImGui::Checkbox("Server Bullet Impacts", &g_cfg.esp.server_bullet_impacts);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 48 + 260 + 48));
                    ImGui::MenuChild("Weather", ImVec2(ChildW, 48));
                    {
                        ImGui::Checkbox("Rain", &g_cfg.esp.rain);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 8)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Camera", ImVec2(ChildW, 48));
                    {
                        ImGui::SliderInt("FOV", &g_cfg.esp.fov, 0, 100);
                        //draw_multicombo("Removals", g_cfg.esp.removals, removals, ARRAYSIZE(removals), preview);
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Viewmodel", ImVec2(ChildW, 171));
                    {
                        ImGui::SliderInt("FOV", &g_cfg.esp.viewmodel_fov, 0, 30);
                        ImGui::SliderInt("X", &g_cfg.esp.viewmodel_x, -30, 30);
                        ImGui::SliderInt("Y", &g_cfg.esp.viewmodel_y, -30, 30);
                        ImGui::SliderInt("Z", &g_cfg.esp.viewmodel_z, -30, 30);
                        ImGui::SliderInt("Roll", &g_cfg.esp.viewmodel_roll, -30, 30);
                        //ImGui::Checkbox("Rare Animations", &g_cfg.skins.rare_animations);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 48 + 48 + 42));
                    ImGui::MenuChild("Thirdperson", ImVec2(ChildW, 80));
                    {
                        draw_keybind(crypt_str("Thirdperson"), &g_cfg.misc.thirdperson_toggle, crypt_str("##TPKEY__HOTKEY"));
                        ImGui::SliderInt("Distance", &g_cfg.misc.thirdperson_distance, 0, 300);
                        ImGui::Checkbox("In Spectators", &g_cfg.misc.thirdperson_when_spectating);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18 + ChildW + 16, 48 + 190 + 22));
                    ImGui::MenuChild("Misc", ImVec2(ChildW, 80));
                    {
                        ImGui::Checkbox("Aspect Ratio", &g_cfg.misc.aspect_ratio);
                        ImGui::SliderFloat("Value", &g_cfg.misc.aspect_ratio_amount, 0.0, 4.0);
                        
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 9)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Movement", ImVec2(ChildW, 200));
                    {
                        ImGui::Checkbox("Auto Jump", &g_cfg.misc.bunnyhop);
                        ImGui::Combo("Auto Strafe", &g_cfg.misc.airstrafe, strafes, ARRAYSIZE(strafes));
                        ImGui::SliderFloat("Turn Speed", &g_cfg.misc.turn_speed, 0.0, 1.0);
                        ImGui::Checkbox("Crouch In Air", &g_cfg.misc.crouch_in_air);
                        ImGui::Checkbox("Quick Stop", &g_cfg.misc.fast_stop);
                        ImGui::Checkbox("Infinity Duck", &g_cfg.misc.noduck);

                        if (g_cfg.misc.noduck)
                            draw_keybind(crypt_str("Fake duck"), &g_cfg.misc.fakeduck_key, crypt_str("##FAKEDUCK__HOTKEY"));

                        draw_keybind(crypt_str("Slow walk"), &g_cfg.misc.slowwalk_key, crypt_str("##SLOWWALK__HOTKEY"));
                        draw_keybind(crypt_str("Auto peek"), &g_cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));
                        draw_keybind(crypt_str("Edge jump"), &g_cfg.misc.edge_jump, crypt_str("##EDGEJUMP__HOTKEY"));
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Other", ImVec2(ChildW, 188));
                    {
                        ImGui::Checkbox("Anti Untrusted", &g_cfg.misc.anti_untrusted);
                        ImGui::Checkbox("Anti Screenshot", &g_cfg.misc.anti_screenshot);

                        draw_multicombo("Event Log", g_cfg.misc.events_to_log, events, ARRAYSIZE(events), preview);
                        draw_multicombo("Log Output", g_cfg.misc.log_output, events_output, ARRAYSIZE(events_output), preview);
                        draw_multicombo("Windows", g_cfg.menu.windows, WindowsType, ARRAYSIZE(WindowsType), preview);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(18, 290));

                    ImGui::MenuChild("Spammers", ImVec2(ChildW, 80));
                    {
                        ImGui::Checkbox("Clantag", &g_cfg.misc.clantag_spammer);
                        ImGui::Checkbox("Chat Spam", &g_cfg.misc.chat);
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(ChildW + 18 + 16, 278));

                    ImGui::MenuChild("BuyBot", ImVec2(ChildW, 143));
                    {
                        ImGui::Checkbox("Enable BuyBot", &g_cfg.misc.buybot_enable);

                        ImGui::Combo("Primary", &g_cfg.misc.buybot1, mainwep, ARRAYSIZE(mainwep));
                        ImGui::Combo("Secondary", &g_cfg.misc.buybot2, secwep, ARRAYSIZE(secwep));
                        draw_multicombo("Equipment", g_cfg.misc.buybot3, grenades, ARRAYSIZE(grenades), preview);
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 11)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Script Selector", ImVec2(ChildW, 270));
                    {
                        
                        static auto should_update = true;

                        if (should_update)
                        {
                            should_update = false;
                            scripts = c_lua::get().scripts;

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }
                        }

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

                        ImGui::SetCursorPosX(25);
                        if (scripts.empty())
                            ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
                        else
                        {
                            auto backup_scripts = scripts;

                            for (auto& script : scripts)
                            {
                                auto script_id = c_lua::get().get_script_id(script + crypt_str(".lua"));

                                if (script_id == -1)
                                    continue;

                                if (c_lua::get().loaded.at(script_id))
                                    scripts.at(script_id) += crypt_str(" [loaded]");
                            }

                            ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
                            scripts = std::move(backup_scripts);
                        }

                        ImGui::PopStyleVar();
                        
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Script Options", ImVec2(ChildW, 300));
                    {
                        
                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Refresh Scripts"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            c_lua::get().refresh_scripts();
                            scripts = c_lua::get().scripts;

                            if (selected_script >= scripts.size())
                                selected_script = scripts.size() - 1; //-V103

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }
                        }

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Load Script"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            c_lua::get().load_script(selected_script);
                            c_lua::get().refresh_scripts();

                            scripts = c_lua::get().scripts;

                            if (selected_script >= scripts.size())
                                selected_script = scripts.size() - 1; //-V103

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }

                            eventlogs::get().add(crypt_str("Loaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
                        }

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Unload Script"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            c_lua::get().unload_script(selected_script);
                            c_lua::get().refresh_scripts();

                            scripts = c_lua::get().scripts;

                            if (selected_script >= scripts.size())
                                selected_script = scripts.size() - 1; //-V103

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }

                            eventlogs::get().add(crypt_str("Unloaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
                        }

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Reload All Scripts"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            c_lua::get().reload_all_scripts();
                            c_lua::get().refresh_scripts();

                            scripts = c_lua::get().scripts;

                            if (selected_script >= scripts.size())
                                selected_script = scripts.size() - 1; //-V103

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }
                        }

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Unload All Scripts"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            c_lua::get().unload_all_scripts();
                            c_lua::get().refresh_scripts();

                            scripts = c_lua::get().scripts;

                            if (selected_script >= scripts.size())
                                selected_script = scripts.size() - 1; //-V103

                            for (auto& current : scripts)
                            {
                                if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
                                    current.erase(current.size() - 5, 5);
                                else if (current.size() >= 4)
                                    current.erase(current.size() - 4, 4);
                            }
                        }
                        
                    }
                    ImGui::EndChild();
                }

                if (ActiveTab == 12)
                {
                    ImGui::SetCursorPos(ImVec2(18, 48));
                    ImGui::MenuChild("Config Selector", ImVec2(ChildW, 270));
                    {
                        static auto should_update = true;

                        if (should_update)
                        {
                            should_update = false;

                            cfg_manager->config_files();
                            files = cfg_manager->files;

                            for (auto& current : files)
                                if (current.size() > 2)
                                    current.erase(current.size() - 3, 3);
                        }

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
                        ImGui::SetCursorPosX(25);
                        ImGui::ListBoxConfigArray(crypt_str(""), &g_cfg.selected_config, files, 9);
                        ImGui::PopStyleVar();

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Refresh Configs"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            cfg_manager->config_files();
                            files = cfg_manager->files;

                            for (auto& current : files)
                                if (current.size() > 2)
                                    current.erase(current.size() - 3, 3);
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine(0, 16);

                    ImGui::MenuChild("Config Options", ImVec2(ChildW, 300));
                    {
                        static char config_name[64] = "\0";

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
                        ImGui::SetCursorPosX(25);
                        ImGui::InputText(crypt_str("##configname"), config_name, sizeof(config_name));
                        ImGui::PopStyleVar();

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Create Config"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                        {
                            g_cfg.new_config_name = config_name;
                            add_config();
                        }

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Save Config"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                            save_config();

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Load Config"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                            load_config();

                        ImGui::SetCursorPosX(25);
                        if (ImGui::Button(crypt_str(""), crypt_str("Remove Config"), ImVec2(225 * dpi_scale, 26 * dpi_scale)))
                            remove_config();
                    }
                    ImGui::EndChild();
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::EndFrame();
    }
}