#include <mix/computer.h>
#include <mix/command.h>
#include <mix/default_device.h>
#include <mixal/program_executor.h>
#include <mixal_parse/types/operation_id.h>
#include <mixui/ui_word.h>
#include <imgui_stdlib.h>

#include <cstdio>

#include <fstream>
#include <iomanip>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <SDL.h>

#include <GL/gl3w.h>

#include <cassert>
#include <cstdlib>

static void SDLCheck(int status)
{
    if (status != 0)
    {
        assert(false && "SDL2 call failed");
        std::abort();
    }
}

static void GLCheck(int status)
{
    if (status != 0)
    {
        assert(false && "OpenGL call failed");
        std::abort();
    }
}

static void ImGuiCheck(bool status)
{
    if (!status)
    {
        assert(false && "ImGui call failed");
        std::abort();
    }
}

struct WordWithSource
{
    mixal::TranslatedWord translated;
    int line_id = -1;
    mixal::OperationId operation_id = mixal::OperationId::Unknown;
};

struct ProgramWithSource
{
    std::vector<WordWithSource> commands;
    int start_address = -1;
};

static void LoadProgram(mix::Computer& computer, const ProgramWithSource& program)
{
    for (const auto& word : program.commands)
    {
        const int address = word.translated.original_address;
        if (address >= 0)
        {
            computer.set_memory(
                word.translated.original_address
                , word.translated.value);
        }
    }
    computer.set_next_address(program.start_address);
}

static ProgramWithSource LoadProgramFromSourceFile(
    const std::string& content, mix::Computer& mix)
{
    std::istringstream in(content);
    mixal::Translator translator;
    mixal::LinesTranslator lines_translator(translator);
    std::vector<mixal::TranslatedLine> lines;

    try
    {
        std::string str;
        while (getline(in, str))
        {
            lines.push_back(lines_translator.translate(str));
            if (lines.back().end_code)
            {
                break;
            }
        }
    }
    catch (const std::exception&)
    {
        return ProgramWithSource();
    }

    ProgramWithSource program;

    for (int line_id = 0, count = static_cast<int>(lines.size()); line_id < count; ++line_id)
    {
        auto word_ref = lines[line_id].word_ref;
        auto end_code = lines[line_id].end_code;
        auto id = lines[line_id].operation_id;
        if (word_ref)
        {
            assert(word_ref->is_ready());
            WordWithSource word;
            word.translated = word_ref->translated_word();
            word.line_id = line_id + 1;
            word.operation_id = id;
            program.commands.push_back(word);
        }
        else if (end_code)
        {
            program.start_address = end_code->start_address;
            for (auto& end_symbols : end_code->defined_symbols)
            {
                WordWithSource word;
                word.translated = end_symbols.second;
                word.line_id = line_id + 1;
                word.operation_id = id;
                program.commands.push_back(word);
            }
        }
        else
        {
            WordWithSource word;
            word.line_id = line_id + 1;
            word.operation_id = id;
            program.commands.push_back(word);
        }
    }

    LoadProgram(mix, program);
    return program;
}

struct UIFlags
{
    int comparison_ = 0;
    bool overflow_ = false;

    static int ComparisonFrom(mix::ComparisonIndicator state)
    {
        switch (state)
        {
        case mix::ComparisonIndicator::Less: return 1;
        case mix::ComparisonIndicator::Greater: return 2;
        case mix::ComparisonIndicator::Equal: return 0;
        }
        return 0;
    }

    void set(mix::ComparisonIndicator state, mix::OverflowFlag overflow)
    {
        comparison_ = ComparisonFrom(state);
        overflow_ = (overflow == mix::OverflowFlag::Overflow);
    }
};

struct UIMix
{
    static constexpr int k_ri_count =
        static_cast<int>(mix::Computer::k_index_registers_count);

    UIMix(mix::Computer& mix)
        : mix_(mix)
    {
    }

    mix::Computer& mix_;
    UIWord ra_;
    UIWord rx_;
    UIWord rj_; // non-negative, bytes [4, 5]
    UIWord ri_[k_ri_count]; // bytes [4, 5]
    int address_ = 0;
    UIFlags flags_;

    std::string bytecode_;
    std::string source_;

    std::stringstream device0_;
    std::string output_;
};

static void PrepareMix(UIMix& ui_mix)
{
    ui_mix.device0_.str(std::string());
    ui_mix.mix_ = mix::Computer();
    ui_mix.mix_.replace_device(18
        , std::make_unique<mix::SymbolDevice>(
            24/*block size*/,
            ui_mix.device0_,
            ui_mix.device0_,
            false));
}

static void PrintCode(const WordWithSource& word, std::ostream& out_)
{
    out_ << std::setw(3) << word.line_id << "| ";

    if (word.translated.original_address < 0)
    {
        out_ << "\n";
        return;
    }

    out_ << std::setw(4) << std::right << word.translated.original_address << ": ";

    const auto prev_fill = out_.fill('0');

    switch (word.operation_id)
    {
    case mixal::OperationId::CON:
    case mixal::OperationId::END:
        out_.fill(' ');
        out_<< '|' << word.translated.value.sign() << '|' << std::setw(14)
            << std::right << word.translated.value.abs_value() << '|' << "\n";
        break;
    case mixal::OperationId::ALF:
        out_ << word.translated.value << "\n";
        break;
    default:
        out_ << mix::Command(word.translated.value) << "\n";
        break;
    }

    out_.fill(prev_fill);
}

static void PrettyPrintBytecode(const ProgramWithSource& program
    , std::string& bytecode)
{
    std::ostringstream out;
    for (const WordWithSource& w : program.commands)
    {
        PrintCode(w, out);
    }
    bytecode = out.str();
}

static void RegistersInputWindow(UIMix& ui_mix)
{
    if (!ImGui::Begin("Registers"))
    {
        ImGui::End();
        return;
    }
    // ImGui::Separator();
    ImGui::Columns(3, nullptr, true);
    ImGui::PushItemWidth(150);
    (void)UIRegisterInput("A", ui_mix.ra_);
    (void)UIRegisterInput("X", ui_mix.rx_);
    (void)UIAddressRegisterInput("J", ui_mix.rj_);
    ImGui::NextColumn();

    auto handle_ri = [&ui_mix](int i)
    {
        char name[32]{};
        (void)snprintf(name, sizeof(name), "I%i", i);
        (void)UIIndexRegisterInput(name, ui_mix.ri_[i - 1]);
    };

    for (int i = 1; i <= 3; ++i)
    {
        handle_ri(i);
    }
    ImGui::NextColumn();

    for (int i = 4; i <= UIMix::k_ri_count; ++i)
    {
        handle_ri(i);
    }

    ImGui::PopItemWidth();
    // ImGui::Separator();
    
    ImGui::End();
}

static bool UIFlagsInput(UIFlags& state)
{
    ImGui::BeginGroup();
    ImGui::PushID("");

    bool changed = false;
    changed |= ImGui::RadioButton("Less", &state.comparison_
        , UIFlags::ComparisonFrom(mix::ComparisonIndicator::Less));
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Equal", &state.comparison_
        , UIFlags::ComparisonFrom(mix::ComparisonIndicator::Equal));
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Greater", &state.comparison_
        , UIFlags::ComparisonFrom(mix::ComparisonIndicator::Greater));
    ImGui::SameLine();
    ImGui::Checkbox("Overflow", &state.overflow_);

    ImGui::PopID();
    ImGui::EndGroup();

    return changed;
}

static bool UIAddressInput(const char* title, int& address)
{
    const int old_address = address;
    if (ImGui::InputInt(title, &address, 0))
    {
        address = std::clamp(address, 0
            , static_cast<int>(mix::Computer::k_memory_words_count));
        return (old_address != address);
    }
    return false;
}

static void UIMenuInput(UIMix& ui_mix)
{
    bool open_file = false;

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                open_file = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (open_file)
    {
        {
            std::ostringstream ss;
            std::ifstream in(R"(C:\dev\mix\src\tests\mixal_code\program_primes.mixal)");
            ss << in.rdbuf();
            ui_mix.source_ = ss.str();
        }
        PrepareMix(ui_mix);
        const auto program = LoadProgramFromSourceFile(ui_mix.source_, ui_mix.mix_);
        PrettyPrintBytecode(program, ui_mix.bytecode_);
    }
}

static void UpdateUIFromMixState(UIMix& ui, const mix::Computer& mix)
{
    ui.address_ = mix.current_address();
    ui.flags_.set(mix.comparison_state(), mix.overflow_flag());
    ui.ra_.set(mix.ra());
    ui.rx_.set(mix.rx());
    ui.rj_.set(mix.rj());
    for (std::size_t i = 1; i <= mix::Computer::k_index_registers_count; ++i)
    {
        ui.ri_[i - 1].set(mix.ri(i));
    }
}

static void RenderAll()
{
    // ImGui::ShowDemoWindow(nullptr);

    static mix::Computer mix;
    static UIMix ui_mix(mix);

    UpdateUIFromMixState(ui_mix, mix);

    RegistersInputWindow(ui_mix);

    if (ImGui::Begin("Rest"))
    {
        (void)UIFlagsInput(ui_mix.flags_);
        (void)UIAddressInput("Address", ui_mix.address_);
    }
    ImGui::End();

    if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_MenuBar))
    {
        UIMenuInput(ui_mix);
        if (!mix.is_halted())
        {
            if (ImGui::Button("Step (F5)"))
            {
                (void)mix.run_one();
            }
            ImGui::SameLine();
            if (ImGui::Button("Run to the end(F5)"))
            {
                (void)mix.run();
            }
        }
    }

    const int address = mix.current_address();
    if ((address >= 0) && (address <= mix::Computer::k_memory_words_count))
    {
        mix::Command command(mix.memory(mix.current_address()));
        const auto id = static_cast<mixal_parse::OperationId>(command.id());
        const std::string_view name = mixal_parse::OperationIdToString(id);
        ImGui::TextUnformatted(name.data(), name.data() + name.size());
    }

    (void)ImGui::InputTextMultiline("##Bytecode", &ui_mix.bytecode_
        , ImVec2(220.f, -1.f), ImGuiInputTextFlags_ReadOnly);

    ImGui::SameLine();

    if (ImGui::InputTextMultiline("##Editor", &ui_mix.source_, ImVec2(-1.f, -1.f)))
    {
        PrepareMix(ui_mix);
        const auto program = LoadProgramFromSourceFile(ui_mix.source_, mix);
        PrettyPrintBytecode(program, ui_mix.bytecode_);
        ui_mix.device0_.str("");
        ui_mix.output_.clear();
    }

    ui_mix.output_ = ui_mix.device0_.str();
    if (!ui_mix.output_.empty())
    {
        if (ImGui::Begin("Output"))
        {
            (void)ImGui::InputTextMultiline("##Output", &ui_mix.output_
                , ImVec2(-1., -1.), ImGuiInputTextFlags_ReadOnly);
        }
        ImGui::End();
    }

    ImGui::End();
}

#if defined(_WIN32)
#include <Windows.h>
#include <tchar.h>
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
#else
int main(int, char**)
#endif
{
    SDLCheck(::SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER));

    // Decide GL+GLSL versions
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDLCheck(::SDL_GL_SetAttribute(
        // Disable GL "debug" mode
        SDL_GL_CONTEXT_FLAGS, 0));
    SDLCheck(::SDL_GL_SetAttribute(
        // OpenGL core profile - deprecated functions are disabled
        SDL_GL_CONTEXT_PROFILE_MASK
        , SDL_GL_CONTEXT_PROFILE_CORE));
    SDLCheck(::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
    SDLCheck(::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0));

    // Create window with graphics context
    SDLCheck(::SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
    SDLCheck(::SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24));
    SDLCheck(::SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8));
    SDL_Window* window = ::SDL_CreateWindow("MIX UI"
        , SDL_WINDOWPOS_CENTERED
        , SDL_WINDOWPOS_CENTERED
        , 1280, 720
        , SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GLContext gl_context = ::SDL_GL_CreateContext(window);
    SDLCheck(::SDL_GL_MakeCurrent(window, gl_context));
    GLCheck(::SDL_GL_SetSwapInterval(1)); // Enable vsync

    // Initialize OpenGL loader
    GLCheck(gl3wInit());

    // Setup Dear ImGui context
    (void)IMGUI_CHECKVERSION();
    (void)ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer bindings
    ImGuiCheck(ImGui_ImplSDL2_InitForOpenGL(window, gl_context));
    ImGuiCheck(ImGui_ImplOpenGL3_Init(glsl_version));

    auto handle_events = [window]
    {
        bool done = false;
        SDL_Event event{};
        while (::SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            done |= (event.type == SDL_QUIT);
            done |= ((event.type == SDL_WINDOWEVENT)
                && (event.window.event == SDL_WINDOWEVENT_CLOSE)
                && (event.window.windowID == ::SDL_GetWindowID(window)));
        }
        return !done;
    };

    auto start_frame = [window]
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
    };

    auto end_frame = [window]
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Render();
        glViewport(0, 0
            , static_cast<int>(io.DisplaySize.x)
            , static_cast<int>(io.DisplaySize.y));
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    };

    while (handle_events())
    {
        start_frame();
        RenderAll();
        end_frame();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
