/* date = June 20th 2020 5:24 pm */

#ifndef MAIN_H
#define MAIN_H


typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;



typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;


#define internal static 
#define persist static
#define global static



#define width 800
#define height 800
#define GAME_NAME "v10"


#define GAME_RES_WIDTH  480
#define GAME_RES_HEIGHT 320
#define GAME_BPP 32
#define FPS_AVG 120
#define TARGET_MICROSECONDS_PER_FRAME  16667ULL


#define WALKING_DOWN_1 0
#define WALKING_DOWN_2 1
#define WALKING_DOWN_3 2

#define WALKING_LEFT_1 3
#define WALKING_LEFT_2 4
#define WALKING_LEFT_3 5

#define WALKING_RIGHT_1 6
#define WALKING_RIGHT_2 7
#define WALKING_RIGHT_3 8

#define WALKING_UP_1 9
#define WALKING_UP_2 10
#define WALKING_UP_3 11


#define DIRECTION_DOWN 0
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 6
#define DIRECTION_UP 9

#define FONT_ROW 98


#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_DEBUG 4

#define LOG_FILE_NAME GAME_NAME".log"


typedef LONG(NTAPI* _NtQueryTimerResolution)  (OUT PULONG MinResolution ,OUT PULONG MaxResolution,OUT PULONG CurrentResulotion);
_NtQueryTimerResolution NtQueryTimerResolution;

typedef struct 
{
    BITMAPINFO BitmapInfo;
    void* memory;
    
}BackBuffer;


typedef struct 
{
    BackBuffer Sprite[12];
    u8 Direction;
    u8 MovementRemain;
    u8 SpriteIndex;
    i16 Position_x;
    i16 Position_y;
}Player;

typedef struct
{
    u8 Blue;
    u8 Green;
    u8 Red; 
    u8 Alpha;
}Pixel32;

typedef struct
{
    u64 FramesRendered;
    float rawFramesPerSec;
    float cookedFramesPerSec;
    i64 frequency;
    BOOL displaydebuginfo;
    ULONG MinTimerRes;
    ULONG MaxTimerRes;
    ULONG currentTimerRes;
    DWORD HandleCount;
    DWORD PrivateBytes;
    PROCESS_MEMORY_COUNTERS_EX MemInfo;
    SYSTEM_INFO systemInfo;
    i64  CurrentSystemtime ;
    i64  PreviousSystemtime;
    float Percent;
}PERFORMANCE;

/*
typedef struct 
{
    IDirectSound8* G_DirectSound;
    IDirectSoundBuffer* G_primaryBuffer; 
    IDirectSoundBuffer8* G_secondaryBuffer; 
}Sound;
*/

/*
typedef struct
{
    char chunkId[4];
    unsigned long chunkSize;
    char format[4];
    unsigned long  subchunksize;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned long  sampleRates;
    unsigned long  bytespersecond;
    unsigned short blockAlign;
    unsigned short bitspersamples;
    char datachunkid [4];
    unsigned long dataSize;
    
}WaveHeaderFile;

*/
typedef struct
{
    DWORD Loglevel;
}REGISTRY;





internal void LogMessageA(_In_ DWORD loglevel , _In_ char* Message , _In_ ...);
internal DWORD LoadRegistry();
internal BOOL AlreadyRunning();
internal DWORD CreateWindowApp();
internal void  ProcessInput();
__forceinline internal void ClearScreen(_In_ __m128i Color);
internal DWORD Load32Bitmap(_In_ char* Filename,_Inout_ BackBuffer *GameBitmap);
internal void Blit_BMP_TO_Buffer(_In_ BackBuffer* bitmap , _In_ u16 x,_In_ u16 y);
internal void Blit_String_To_Buffer(_In_ char* string, _In_ BackBuffer* bitmap,_In_ u16 x,_In_ u16 y,_In_ Pixel32* color);
internal void Render_Main_Menu();
internal DWORD InitPlayer();


internal void RenderGraphics();
int CALLBACK WinMain(_In_ HINSTANCE Instance, _In_ HINSTANCE PrevInstance,
                     _In_ LPSTR CmdLine, _In_ int CmdShow);




#endif //MAIN_H
