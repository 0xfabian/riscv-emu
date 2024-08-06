#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>
#include <time.h>
#include <cpu.h>

#define MEMORY_SIZE         0x100000
#define SCREEN_ADDRESS      0x10000
#define KEYBOARD_ADDRESS    0x09000
#define RANDOM_ADDRESS      0x09002
#define STACK_POINTER       0x20000
#define FB_WIDTH            32
#define FB_HEIGHT           16

#define WHITE       { 255, 255, 255 }
#define GREY        { 128, 128, 128 }
#define YELLOW      { 255, 255, 0 }
#define SP_COLOR    { 0, 212, 92 }
#define PC_COLOR    { 7, 77, 181 }
#define RA_COLOR    { 217, 43, 43 }

uint8_t memory[MEMORY_SIZE];
CPU cpu(memory);

SDL_Rect screen;
uint8_t framebuffer[3 * FB_WIDTH * FB_HEIGHT];
int memory_view = 0;

SDL_Window* win;
SDL_Renderer* ren;
bool quit = false;
bool autostep = false;
bool fullscreen = true;

TTF_Font* font;
int font_width;
int font_height;

SDL_Texture* font_charset;
char charset[] = { " !\"#$%&'()*+,-./0123456789:; = ?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" };

void generate_charset(SDL_Renderer* ren)
{
    SDL_Surface* text_surface = TTF_RenderText_Blended(font, charset, WHITE);
    font_charset = SDL_CreateTextureFromSurface(ren, text_surface);

    SDL_FreeSurface(text_surface);
    SDL_QueryTexture(font_charset, nullptr, nullptr, &font_width, &font_height);

    font_width /= strlen(charset);
}

void render_text(SDL_Renderer* ren, int x, int y, std::string str, SDL_Color color)
{
    SDL_Rect src;
    src.x = 0;
    src.y = 0;
    src.w = font_width;
    src.h = font_height;

    SDL_Rect dest;
    dest.x = x;
    dest.y = y;
    dest.h = font_height;
    dest.w = font_width;

    SDL_SetTextureColorMod(font_charset, color.r, color.g, color.b);

    for (auto ch : str)
    {
        if (ch == '\n')
        {
            dest.x = x;
            dest.y += font_height;
        }
        else
        {
            src.x = (ch - ' ') * font_width;
            SDL_RenderCopy(ren, font_charset, &src, &dest);
            dest.x += font_width;
        }
    }
}

void init_all(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "use: %s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    win = SDL_CreateWindow("RISC-V Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, 0);

    if (win == nullptr)
    {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    if (ren == nullptr)
    {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    font = TTF_OpenFont("font.ttf", 20);

    if (font == nullptr)
    {
        fprintf(stderr, "Could not open font\n");
        exit(EXIT_FAILURE);
    }

    generate_charset(ren);

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        fprintf(stderr, "Could not open `%s`\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    std::streamsize size = file.tellg();

    if (size > MEMORY_SIZE)
    {
        fprintf(stderr, "Not enough memory");
        exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::beg);
    file.read((char*)memory, size);
    file.close();

    srand(time(nullptr));
}

void clean_all()
{
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
}

void render_registers(int x, int y)
{
    for (int i = 0; i < 16; i++)
    {
        int j = i + 16;

        SDL_Color i_color = WHITE;
        SDL_Color j_color = WHITE;

        if (i == cpu.dirty)
            i_color = YELLOW;
        else if (i == 2)
            i_color = SP_COLOR;
        else if (i == 1)
            i_color = RA_COLOR;

        if (j == cpu.dirty)
            j_color = YELLOW;
        else    if (j == 2)
            j_color = SP_COLOR;
        else if (j == 1)
            j_color = RA_COLOR;

        std::string i_str = fmt("%-4s = %08x", reg_name[i], cpu.x[i]);
        std::string j_str = fmt("%-4s = %08x", reg_name[j], cpu.x[j]);

        render_text(ren, x, y + i * font_height, i_str, i_color);
        render_text(ren, x + 230, y + i * font_height, j_str, j_color);
    }

    std::string pc_str = fmt("pc   = %08x", cpu.pc);
    render_text(ren, x, y + 16 * font_height, pc_str, PC_COLOR);
}

void render_memory(int x, int y, int offset)
{
    int rows = (800 - x) / font_height;

    if ((cpu.pc - offset) / 16 < rows)
    {
        SDL_Rect rect;
        rect.x = x + font_width * 9.5 + (cpu.pc % 16) * font_width * 3;
        rect.y = y + ((cpu.pc - offset) / 16) * font_height;
        rect.w = font_width * 12;
        rect.h = font_height;

        SDL_Color col = PC_COLOR;
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
        SDL_RenderFillRect(ren, &rect);
    }

    if ((cpu.x[1] - offset) / 16 < rows)
    {
        SDL_Rect rect;
        rect.x = x + font_width * 9.5 + (cpu.x[1] % 16) * font_width * 3;
        rect.y = y + ((cpu.x[1] - offset) / 16) * font_height;
        rect.w = font_width * 12;
        rect.h = font_height;

        SDL_Color col = RA_COLOR;
        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);

        if (cpu.x[1] == cpu.pc)
            SDL_RenderDrawRect(ren, &rect);
        else
            SDL_RenderFillRect(ren, &rect);
    }

    std::string memory_str = "";
    uint8_t* ptr = memory + offset;

    for (int i = 0; i < rows; i++)
    {
        if (ptr >= memory + MEMORY_SIZE)
            break;

        memory_str += fmt("%08x: ", (uint32_t)(ptr - memory));

        for (int j = 0; j < 16; j++)
            memory_str += fmt("%02x ", *ptr++);

        memory_str += "\n";
    }

    render_text(ren, x, y, memory_str, WHITE);
}

void render_instruction(int x, int y)
{
    for (int i = -2; i <= 2; i++)
    {
        int address = cpu.pc + i * sizeof(uint32_t);

        if (address < 0 || address >= MEMORY_SIZE)
            continue;

        SDL_Color col = GREY;

        if (i == 0)
            col = WHITE;

        render_text(ren, x, y + (i + 2) * font_height, cpu.disassemble(*((uint32_t*)&memory[address])), col);
    }
}

void render_screen(SDL_Rect* screen)
{
    for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++)
    {
        framebuffer[3 * i] = memory[SCREEN_ADDRESS + i];
        framebuffer[3 * i + 1] = memory[SCREEN_ADDRESS + i];
        framebuffer[3 * i + 2] = memory[SCREEN_ADDRESS + i];
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)framebuffer,
        FB_WIDTH,
        FB_HEIGHT,
        3 * 8,          // bits per pixel = 24
        FB_WIDTH * 3,   // pitch
        0x0000FF,       // red mask
        0x00FF00,       // green mask
        0xFF0000,       // blue mask
        0);

    SDL_Texture* screen_tex = SDL_CreateTextureFromSurface(ren, surface);
    SDL_RenderCopy(ren, screen_tex, nullptr, screen);

    SDL_DestroyTexture(screen_tex);
    SDL_FreeSurface(surface);

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderDrawRect(ren, screen);
}

int main(int argc, char** argv)
{
    init_all(argc, argv);

    cpu.reset();
    cpu.pc = 0;
    cpu.x[2] = STACK_POINTER;

    SDL_Event event;

    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
                break;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_SPACE:    autostep = !autostep;                   break;
                case SDLK_TAB:      fullscreen = !fullscreen;               break;
                case SDLK_RETURN:   cpu.step();                             break;
                case SDLK_UP:       memory[KEYBOARD_ADDRESS] = -1;          break;
                case SDLK_DOWN:     memory[KEYBOARD_ADDRESS] = 1;           break;
                case SDLK_LEFT:     memory[KEYBOARD_ADDRESS + 1] = -1;      break;
                case SDLK_RIGHT:    memory[KEYBOARD_ADDRESS + 1] = 1;       break;
                case SDLK_HOME:     memory_view = 0;                        break;
                case SDLK_END:      memory_view = MEMORY_SIZE - 16;         break;
                case SDLK_PAGEUP:
                {
                    if (memory_view - 16 >= 0)
                        memory_view -= 16;

                    break;
                }
                case SDLK_PAGEDOWN:
                {
                    if (memory_view + 16 < MEMORY_SIZE)
                        memory_view += 16;

                    break;
                }
                case SDLK_BACKSPACE:
                {
                    cpu.reset();
                    cpu.pc = 0;
                    cpu.x[2] = STACK_POINTER;
                }
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:
                case SDLK_DOWN:     memory[KEYBOARD_ADDRESS] = 0;           break;
                case SDLK_LEFT:
                case SDLK_RIGHT:    memory[KEYBOARD_ADDRESS + 1] = 0;       break;
                }
            }
        }

        memory[RANDOM_ADDRESS] = rand();

        if (autostep)
            cpu.step();

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        if (fullscreen)
        {
            screen.w = FB_WIDTH * 32;
            screen.h = FB_HEIGHT * 32;
            screen.x = 600 - screen.w / 2;
            screen.y = 400 - screen.h / 2;
        }
        else
        {
            screen.x = 750;
            screen.y = 16 + (17 + 1 + 5 + 1) * font_height;
            screen.w = FB_WIDTH * 8;
            screen.h = FB_HEIGHT * 8;

            render_registers(750, 16);
            render_memory(32, 16, memory_view);
            render_instruction(750, 16 + (17 + 1) * font_height);
        }

        render_screen(&screen);

        SDL_RenderPresent(ren);
    }

    clean_all();

    return 0;
}