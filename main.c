#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <emmintrin.h>
#include <psapi.h>
#include "main.h"








//GLOBAL VARIABLES
global BOOL G_running = TRUE;
global HWND G_Window;
global BackBuffer G_BackBuffer = {0};
global PERFORMANCE G_Performance = {0};
global i16 G_Monitorwidth;
global i16 G_Monitorheigth;
global Player G_Player = {0};
MONITORINFO G_MonitorInfo = {sizeof(MONITORINFO)};
global BOOL G_windowfocus;
global BackBuffer G_6x7Font = {0};
global REGISTRY G_Registry = {0};
global BOOL G_StartGame = FALSE;
//global Sound G_Sound = {0};





internal void LogMessageA(_In_ DWORD loglevel , _In_ char* Message , _In_ ...)
{
    size_t messagelength = (size_t)strlen(Message);
    SYSTEMTIME Time = {0};
    HANDLE LogFileHandle = INVALID_HANDLE_VALUE;
    DWORD endoffile = 0;
    DWORD numberofbyteswritten = 0;
    
    char DateTimeString[96] = {0};
    char severityString[8] = {0};
    char formattedString [4096] = {0};
    int error = 0;
    
    
    if(G_Registry.Loglevel<loglevel)
    {
        return ;
    }
    if (messagelength <1 && messagelength > 4096)
    {
        
        return ;
    }
    switch(loglevel)
    {
        case LOG_LEVEL_NONE:
        {
            return;
        }
        case LOG_LEVEL_INFO:
        {
            strcpy_s(severityString,sizeof(severityString),"[INFO]");
        }break;
        case LOG_LEVEL_WARN:
        {
            strcpy_s(severityString,sizeof(severityString),"[WARN]");
        }break;
        case LOG_LEVEL_ERROR:
        {
            strcpy_s(severityString,sizeof(severityString),"[ERROR]");
        }break;
        case LOG_LEVEL_DEBUG:
        {
            strcpy_s(severityString,sizeof(severityString),"[DEBUG]");
        }break;
        default: {}break;
    }
    
    
    GetLocalTime(&Time);
    va_list ArgPointer = NULL;
    va_start(ArgPointer,Message);
    _vsnprintf_s(formattedString,sizeof(formattedString),_TRUNCATE,Message,ArgPointer);
    va_end(ArgPointer);
    error = _snprintf_s(DateTimeString,sizeof(DateTimeString),_TRUNCATE,"\r\n[%02u/%02u/%u  %02u:%02u:%02u.%03u]",Time.wMonth,Time.wDay,Time.wYear,Time.wHour,Time.wMinute,Time.wSecond,Time.wMilliseconds);
    if((LogFileHandle = CreateFile(LOG_FILE_NAME,FILE_APPEND_DATA,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL)) == INVALID_HANDLE_VALUE)
    {
        
        return;
    }
    
    
    endoffile = SetFilePointer(LogFileHandle , 0,NULL,FILE_END);
    WriteFile(LogFileHandle,DateTimeString,(DWORD)strlen(DateTimeString),&numberofbyteswritten,NULL);
    WriteFile(LogFileHandle,severityString,(DWORD)strlen(severityString),&numberofbyteswritten,NULL);
    WriteFile(LogFileHandle,formattedString,(DWORD)strlen(formattedString),&numberofbyteswritten,NULL);
    
    if(LogFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(LogFileHandle);
    }
}


internal DWORD LoadRegistry()
{
    DWORD result = ERROR_SUCCESS;
    HKEY Regkey = NULL;
    DWORD disposition = 0;
    DWORD bytesread = sizeof(DWORD);
    result = RegCreateKeyExA(HKEY_CURRENT_USER,"SOFTWARE\\" GAME_NAME,0,NULL,0,
                             KEY_ALL_ACCESS,NULL,&Regkey,&disposition);
    if(result != ERROR_SUCCESS)
    {
        LogMessageA(LOG_LEVEL_ERROR ,"(%s)RegCreateKey failed with error code 0x%08lx!",__FUNCTION__,result);
        goto Exit;
    }
    
    if(disposition == REG_CREATED_NEW_KEY)
    {
        LogMessageA(LOG_LEVEL_INFO,"(%s) Registry key did not exist ; created new key HKCU\\SOFTWARE\\%s.",__FUNCTION__,GAME_NAME);
    }
    else
    {
        LogMessageA(LOG_LEVEL_INFO,"(%s) Opened existing registry key HKCU\\SOFTWARE\\%s",__FUNCTION__,GAME_NAME);
    }
    
    result = RegGetValueA(Regkey,NULL,"LogLevel",RRF_RT_DWORD,NULL,(BYTE*)&G_Registry.Loglevel,&bytesread);
    if(result != ERROR_SUCCESS)
    {
        if(result == ERROR_FILE_NOT_FOUND)
        {
            result = ERROR_SUCCESS;
            LogMessageA(LOG_LEVEL_INFO,"(%s) Registry value 'loglevel' not found . using default of 0. (LOG_LEVEL_NONE)",__FUNCTION__);
            G_Registry.Loglevel = LOG_LEVEL_NONE;
        }
        else
        {
            LogMessageA(LOG_LEVEL_ERROR,"(%s) failed to read 'loglevel' registry value error 0x%08lx!",__FUNCTION__,result);
            goto Exit;
        }
    }
    LogMessageA(LOG_LEVEL_INFO,"[%s] loglevel is %d.",__FUNCTION__,G_Registry.Loglevel);
    //...
    
    
    Exit:
    
    if(Regkey)
    {
        RegCloseKey(Regkey);
    }
    return(result);
}

internal BOOL AlreadyRunning()
{
    HANDLE mutex=NULL;
    mutex =CreateMutexA(NULL,FALSE,GAME_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {return (TRUE);}
    else
    {return(FALSE);}
}

LRESULT CALLBACK windproc (_In_ HWND WindowHandle , _In_ UINT c ,_In_ WPARAM wp ,_In_ LPARAM lp )
{
    
    
    switch(c)
    {
        
        case WM_CLOSE : 
        {
            
            PostQuitMessage(0);
            G_running = FALSE;
            G_StartGame = TRUE;
        }
        break;
        case WM_ACTIVATE:
        {
            if(wp == 0)
            {
                G_windowfocus = FALSE;
            }
            else
            {
                G_windowfocus = TRUE;
                ShowCursor(FALSE);
            }
            
        }
        break;
        
        default : return DefWindowProc(WindowHandle , c , wp , lp);
        
    }
    return (0);
}

internal DWORD CreateWindowApp()
{
    DWORD result = ERROR_SUCCESS;
    
    WNDCLASSEXA Window = { 0 };
    HWND hwnd;
    Window.cbSize        = sizeof(WNDCLASSEX); 
    Window.lpfnWndProc   = windproc;
    Window.hInstance     = GetModuleHandle(NULL);
    Window.lpszClassName = GAME_NAME;
    Window.hbrBackground = CreateSolidBrush(RGB(0,0,20));
    
    if (!RegisterClassExA(&Window))
    {
        result = GetLastError();
        MessageBoxA(NULL,"error ","error in regsiter class",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    G_Window=CreateWindowExA(0,GAME_NAME,GAME_NAME,WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                             width,height,NULL,NULL,Window.hInstance,NULL);
    if ( G_Window == NULL)
    {
        result = GetLastError();
        MessageBox(NULL,"error ","error in creating  window ",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    
    GetMonitorInfoA(MonitorFromWindow(G_Window,MONITOR_DEFAULTTOPRIMARY),&G_MonitorInfo);
    G_Monitorwidth = G_MonitorInfo.rcMonitor.right  -G_MonitorInfo.rcMonitor.left;
    G_Monitorheigth= G_MonitorInfo.rcMonitor.bottom -G_MonitorInfo.rcMonitor.top;
    
    if(SetWindowLongPtrA(G_Window,GWL_STYLE,WS_VISIBLE)== 0)
    {
        result = GetLastError();
        goto Exit;
    }
    if(SetWindowPos(G_Window,HWND_TOP,G_MonitorInfo.rcMonitor.left,
                    G_MonitorInfo.rcMonitor.top,G_Monitorwidth,
                    G_Monitorheigth,SWP_FRAMECHANGED)==0)
    {
        result = GetLastError();
        goto Exit;
    }
    
    
    Exit:
    return (result);
    
}

internal void ProcessInput()
{
    if (G_windowfocus == FALSE)
    {return ;}
    
    i16 EscapeKeyisDown = GetAsyncKeyState(VK_ESCAPE);
    i16 DebugInfoIsDown = GetAsyncKeyState(VK_F2);
    i16 RightKeysIsDown = GetAsyncKeyState('D');
    i16 LeftKeyIsDown = GetAsyncKeyState('Q');
    i16 DownKeyIsDown = GetAsyncKeyState('S');
    i16 UpKeyIsDown = GetAsyncKeyState('Z');
    i16 EnterKeyisDown = GetAsyncKeyState(VK_RETURN);
    
    persist i16 debugkeywasdown;
    
    if(EnterKeyisDown && (G_StartGame ==FALSE))
    {
        G_StartGame = TRUE;
    }
    
    if (EscapeKeyisDown)
    {
        SendMessageA(G_Window,WM_CLOSE,0,0);
    }
    if (EscapeKeyisDown && (G_StartGame == TRUE))
    {
        G_StartGame = FALSE ;
    }
    if (DebugInfoIsDown && !debugkeywasdown)
    {
        G_Performance.displaydebuginfo = !G_Performance.displaydebuginfo ;
    }
    
    debugkeywasdown = DebugInfoIsDown;
    
    
    if (!G_Player.MovementRemain)
    {
        
        if (DownKeyIsDown)
        {
            if(G_Player.Position_y+16 < GAME_RES_HEIGHT)
            {
                G_Player.Direction = DIRECTION_DOWN;
                G_Player.MovementRemain = 16;
            }
        }
        else if (LeftKeyIsDown)
        {
            if(G_Player.Position_x > 0)
            {
                G_Player.MovementRemain = 16;
                G_Player.Direction = DIRECTION_LEFT;
            }
        }
        else if (RightKeysIsDown)
        {
            if(G_Player.Position_x+16 < GAME_RES_WIDTH)
            {
                G_Player.MovementRemain = 16;
                G_Player.Direction = DIRECTION_RIGHT;
            }
        }
        else if (UpKeyIsDown)
        {
            if(G_Player.Position_y > 0)
            {
                G_Player.MovementRemain = 16;
                G_Player.Direction = DIRECTION_UP;
            }
        }
    }
    else
    {
        G_Player.MovementRemain--;
        if (G_Player.Direction == DIRECTION_DOWN)
        {
            
            G_Player.Position_y++;
        }
        else if (G_Player.Direction == DIRECTION_LEFT)
        {
            
            G_Player.Position_x--;
        }
        else if (G_Player.Direction == DIRECTION_RIGHT)
        {
            
            G_Player.Position_x++;
        }
        else if (G_Player.Direction == DIRECTION_UP)
        {
            
            G_Player.Position_y--;
        }
        switch(G_Player.MovementRemain)
        {
            case 16:{ G_Player.SpriteIndex = 0;} break;
            case 12: {G_Player.SpriteIndex = 1;} break;
            case 8 :{G_Player.SpriteIndex  = 0;} break;
            case 4 :{G_Player.SpriteIndex  = 2;} break;
            case 0 :{G_Player.SpriteIndex  = 0;} break;
            default : break;
        }
        
    }
    
}

__forceinline internal void ClearScreen(_In_ __m128i Color)
{
    
    for (int i = 0;i < GAME_RES_WIDTH * GAME_RES_HEIGHT;i+=4)
    {
        _mm_store_si128((Pixel32*)G_BackBuffer.memory + i,Color);
        
    }
}




internal DWORD Load32Bitmap(_In_ char* Filename ,_Inout_ BackBuffer* GameBitmap)
{
    DWORD error = ERROR_SUCCESS;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    WORD BitMapHeader = 0;
    DWORD PixelData = 0;
    DWORD NumberofBytes =2;
    
    if((fileHandle = CreateFileA(Filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL))
       == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        goto Exit;
    }
    
    if(ReadFile(fileHandle,&BitMapHeader,2,&NumberofBytes,NULL) == 0)
    {
        error = GetLastError();
        goto Exit;
    }
    
    if(BitMapHeader != 0x4d42 )
    {
        error = ERROR_FILE_INVALID;
        goto Exit;
    }
    if(SetFilePointer(fileHandle,0xA,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        error = GetLastError();
        goto Exit;
    }
    
    if(ReadFile(fileHandle,&PixelData,sizeof(DWORD),&NumberofBytes,NULL) == 0)
    {
        error = GetLastError();
        goto Exit;
    }
    
    if(SetFilePointer(fileHandle,0xE,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        error = GetLastError();
        goto Exit;
    }
    if(ReadFile(fileHandle,&GameBitmap->BitmapInfo.bmiHeader,sizeof(BITMAPINFOHEADER),&NumberofBytes,NULL)==0)
    {
        error = GetLastError();
        goto Exit;
    }
    
    
    if((GameBitmap->memory = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,GameBitmap->BitmapInfo.bmiHeader.biSizeImage))== NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    
    if(SetFilePointer(fileHandle,PixelData,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        error = GetLastError();
        goto Exit;
    }
    
    if(ReadFile(fileHandle,GameBitmap->memory,GameBitmap->BitmapInfo.bmiHeader.biSizeImage,&NumberofBytes,NULL)== 0)
    {
        error = GetLastError();
        goto Exit;
    }
    
    Exit:
    if(fileHandle && (fileHandle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(fileHandle);
    }
    return (error);
}







internal void Blit_BMP_TO_Buffer(_In_ BackBuffer* bitmap,_In_ u16 x ,_In_ u16 y)
{
    i32 startingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT)-GAME_RES_WIDTH) - (GAME_RES_WIDTH * y) + x;
    i32 startingBitmap = ((bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight) - bitmap->BitmapInfo.bmiHeader.biWidth);
    i32 bitmapoffset = 0;
    i32 memoryoffset = 0;
    Pixel32 BitmapPixel = {0};
    //Pixel32 BitmapPixel = {0};
    
    for(i16 ypixel = 0 ; ypixel<bitmap->BitmapInfo.bmiHeader.biHeight; ypixel++)
    {
        for (i16 xpixel = 0 ; xpixel< bitmap->BitmapInfo.bmiHeader.biWidth ; xpixel++)
        {
            memoryoffset = startingScreenPixel + xpixel -(GAME_RES_WIDTH * ypixel);
            bitmapoffset = startingBitmap + xpixel -(bitmap->BitmapInfo.bmiHeader.biWidth * ypixel);
            memcpy_s(&BitmapPixel,sizeof(Pixel32),(Pixel32*)bitmap->memory + bitmapoffset,sizeof(Pixel32));
            if(BitmapPixel.Alpha == 255)
            {
                memcpy_s((Pixel32*)G_BackBuffer.memory + memoryoffset,sizeof(Pixel32),&BitmapPixel,sizeof(Pixel32));
            }
        }
    }
}


internal void Blit_String_To_Buffer(_In_ char* string , _In_ BackBuffer* bitmap,_In_ u16 x, _In_ u16 y,_In_ Pixel32* color)
{
    u16 charwidth  = (u16)bitmap->BitmapInfo.bmiHeader.biWidth / FONT_ROW;
    u16 charheight = (u16)bitmap->BitmapInfo.bmiHeader.biHeight;
    u16 byteschar  = ((charwidth * charheight) * (bitmap->BitmapInfo.bmiHeader.biBitCount /8));
    u16 stringlength = (u16)strlen(string);
    
    BackBuffer stringbitmap ={0};
    
    stringbitmap.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;
    stringbitmap.BitmapInfo.bmiHeader.biHeight   = charheight;
    stringbitmap.BitmapInfo.bmiHeader.biWidth    = charwidth * stringlength;
    stringbitmap.BitmapInfo.bmiHeader.biPlanes   = 1;
    stringbitmap.BitmapInfo.bmiHeader.biCompression = BI_RGB;
    stringbitmap.memory = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY ,((size_t)byteschar * (size_t)stringlength));
    
    
    for(int i = 0; i <stringlength ;i++)
    {
        int startingfont = 0;
        int fontoffset   = 0;
        int stringbitmapoffset = 0;
        Pixel32 fontpixel = {0};
        
        switch(string[i])
        {
            case 'A':
            {
                startingfont =( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth  ;
            }break;
            case 'B':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + charwidth ;
            }break;
            case 'C':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 2);
            }break;
            case 'D':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 3);
            }break;
            case 'E':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 4);
            }break;
            case 'F':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 5);
            }break;
            case 'G':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 6);
            }break;
            case 'H':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 7);
            }break;
            case 'I':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 8);
            }break;
            case 'J':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 9);
            }break;
            case 'K':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 10);
            }break;
            case 'L':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 11);
            }break;
            case 'M':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 12);
            }break;
            case 'N':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 13);
            }break;
            case 'O':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 14);
            }break;
            case 'P':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 15);
            }break;
            case 'Q':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 16);
            }break;
            case 'R':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 17);
                
            }break;
            case 'S':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 18);
            }break;
            case 'T':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 19);
            }break;
            case 'U':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 20);
            }break;
            case 'V':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 21);
            }break;
            case 'W':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 22);
            }break;
            case 'X':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 23);
            }break;
            case 'Y':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 24);
            }break;
            case 'Z':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 25);
            }break;
            case 'a':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 26);
            }break;
            case 'b':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 27);
            }break;
            case 'c':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 28);
            }break;
            case 'd':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 29);
            }break;
            case 'e':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 30);
            }break;
            case 'f':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 31);
            }break;
            case 'g':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 32);
            }break;
            case 'h':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 33);
            }break;
            case 'i':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 34);
            }break;
            case 'j':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 35);
            }break;
            case 'k':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 36);
            }break;
            case 'l':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 37);
            }break;
            case 'm':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 38);
            }break;
            case 'n':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 39);
            }break;
            case 'o':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 40);
            }break;
            case 'p':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 41);
            }break;
            case 'q':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 42);
            }break;
            case 'r':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 43);
            }break;
            case 's':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 44);
            }break;
            case 't':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 45);
            }break;
            case 'u':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 46);
            }break;
            case 'v':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 47);
            }break;
            case 'w':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 48);
            }break;
            case 'x':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 49);
            }break;
            case 'y':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 50);
            }break;
            case 'z':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 51);
            }break;
            case '0':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 52);
            }break;
            case '1':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 53);
            }break;
            case '2':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 54);
            }break;
            case '3':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 55);
            }break;
            case '4':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 56);
            }break;
            case '5':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 57);
            }break;
            case '6':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 58);
            }break;
            case '7':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 59);
            }break;
            case '8':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 60);
            }break;
            case '9':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 61);
            }break;
            case '%':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 68);
            }break;
            case '?':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 93);
            }break;
            case ' ':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 94);
            }break;
            case '.':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 91);
            }break;
            default:
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 93);
            }break;
            case ':':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 86);
            }break;
            case ',':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 88);
            }break;
            case '}':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 83);
            }break;
            case '+':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 77);
            }break;
            case '_':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 76);
            }break;
            case '=':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 75);
            }break;
            case '-':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 74);
            }break;
            case ')':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 73);
            }break;
            case '(':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 72);
            }break;
            case '*':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 71);
            }break;
            case '&':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 71);
            }break;
            case '^':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 70);
            }break;
            
            case '$':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 68);
            }break;
            case '#':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 67);
            }break;
            case '!':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 65);
            }break;
            case '~':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 64);
            }break;
            case '{':
            {
                startingfont =(( bitmap->BitmapInfo.bmiHeader.biWidth * bitmap->BitmapInfo.bmiHeader.biHeight)-   bitmap->BitmapInfo.bmiHeader.biWidth) + (charwidth * 82);
            }break;
        }
        for(u16 rowpixel = 0; rowpixel < charheight;rowpixel++)
        {
            for (u16 colonpixel = 0;colonpixel < charwidth;colonpixel++ )
            {
                fontoffset = startingfont + colonpixel - (bitmap->BitmapInfo.bmiHeader.biWidth * rowpixel);
                stringbitmapoffset =(i*charwidth)+((stringbitmap.BitmapInfo.bmiHeader.biWidth *stringbitmap.BitmapInfo.bmiHeader.biHeight)- stringbitmap.BitmapInfo.bmiHeader.biWidth ) + colonpixel -(stringbitmap.BitmapInfo.bmiHeader.biWidth) * rowpixel;
                
                memcpy_s(&fontpixel,sizeof(Pixel32),(Pixel32*)bitmap->memory + fontoffset,sizeof(Pixel32));
                
                if(fontpixel.Alpha == 255)
                {
                    fontpixel.Blue  = color->Blue;
                    fontpixel.Green = color->Green;
                    fontpixel.Red   = color->Red;
                    fontpixel.Alpha = color->Alpha;
                }
                memcpy_s((Pixel32*)stringbitmap.memory+stringbitmapoffset,sizeof(Pixel32),&fontpixel,sizeof(Pixel32));
                
                
            }
        }
    }
    
    Blit_BMP_TO_Buffer(&stringbitmap,x,y);
    
    
    
    if(stringbitmap.memory)
    {
        HeapFree(GetProcessHeap(),0,stringbitmap.memory);
    }
    
}

internal DWORD InitPlayer()
{
    DWORD error = ERROR_SUCCESS;
    
    G_Player.Position_x = 96;
    G_Player.Position_y = 96;
    G_Player.Direction = DIRECTION_DOWN;
    G_Player.SpriteIndex = WALKING_DOWN_1 ;
    G_Player.MovementRemain = 0;
    
    // LOAD DOWN BITMAPS
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerStand1.bmpx",
                             &G_Player.Sprite[WALKING_DOWN_1])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerDown1.bmpx",
                             &G_Player.Sprite[WALKING_DOWN_2])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerDown2.bmpx",
                             &G_Player.Sprite[WALKING_DOWN_3])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    
    
    // LOAD LEFT BITMAPS
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerLeft1.bmpx",
                             &G_Player.Sprite[WALKING_LEFT_1])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerLeft2.bmpx",
                             &G_Player.Sprite[WALKING_LEFT_2])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerLeft3.bmpx",
                             &G_Player.Sprite[WALKING_LEFT_3])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    
    
    // LOAD RIGHT BITMAPS
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerRight1.bmpx",
                             &G_Player.Sprite[WALKING_RIGHT_1])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerRight2.bmpx",
                             &G_Player.Sprite[WALKING_RIGHT_2])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerRight3.bmpx",
                             &G_Player.Sprite[WALKING_RIGHT_3])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    
    
    // LOAD UP BITMAPS
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerUp1.bmpx",
                             &G_Player.Sprite[WALKING_UP_1])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerUp2.bmpx",
                             &G_Player.Sprite[WALKING_UP_2])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    if((error = Load32Bitmap("C:\\hosni\\projet\\data\\PlayerUp3.bmpx",
                             &G_Player.Sprite[WALKING_UP_3])) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto Exit;
    }
    
    
    Exit:
    return (error);
}


internal void RenderGraphics()
{ 
    
    
    
    __m128i qpixel= {0x7f,0x00,0x00,0xff,0x7f,0x00,0x00,0xff,0x7f,0x00,0x00,0xff,0x7f,0x00,0x00,0xff};
    
    
    ClearScreen(qpixel);
    
    
#if 1
    i32 screenx = G_Player.Position_x ;
    i32 screeny = G_Player.Position_y;
    
    //i32 startingpoint =
    //((GAME_RES_WIDTH*GAME_RES_HEIGHT)-GAME_RES_WIDTH - (GAME_RES_WIDTH*screeny)+screenx );
    
    
    
    Pixel32 color= {0};
    /*color.Blue  = screenx;
    color.Green  = screeny;
    color.Red  = 0;
    color.Alpha  = 255;
*/
    persist u8 position = 50; 
    
    //Blit_String_To_Buffer("V10 ENGINE",&G_6x7Font,position++,position,&color);
    
    Blit_BMP_TO_Buffer(&G_Player.Sprite[G_Player.Direction + G_Player.SpriteIndex],G_Player.Position_x,G_Player.Position_y);
    
    if (G_Performance.displaydebuginfo == TRUE)
    {
        //SelectObject(DeviceContext,(HFONT)GetStockObject(ANSI_FIXED_FONT));
        
        char debuginfobuffer [64] = {0};
        
        
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "FPS Raw : %.01f",G_Performance.rawFramesPerSec);
        //TextOutA(DeviceContext,0,0,debuginfobuffer,(int)strlen(debuginfobuffer));
        color.Blue = 0;
        color.Green = 255;
        color.Red = 255;
        color.Alpha = 255;
        
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,0,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "FPS Cooked : %.01f",G_Performance.cookedFramesPerSec);
        //TextOutA(DeviceContext,0,14,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,10,&color);
        
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "MinTimerRes: %.02f",G_Performance.MinTimerRes/10000.0f);
        //TextOutA(DeviceContext,0,28,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,20,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "MaxTimerRes: %.02f",G_Performance.MaxTimerRes/10000.0f);
        //TextOutA(DeviceContext,0,42,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,30,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "currentTimerRes : %.02f",G_Performance.currentTimerRes/10000.0f);
        //TextOutA(DeviceContext,0,56,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,40,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "Position_ : {%i,%i}",G_Player.Position_x,G_Player.Position_y);
        //TextOutA(DeviceContext,0,70,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,50,&color);
        
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "Handles : %lu",G_Performance.HandleCount);
        //TextOutA(DeviceContext,0,84,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,60,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "Memory : %llu kb",G_Performance.MemInfo.PrivateUsage/1024);
        //TextOutA(DeviceContext,0,98,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,70,&color);
        sprintf_s(debuginfobuffer,sizeof (debuginfobuffer),
                  "CPU: %.01f%%",G_Performance.Percent);
        //TextOutA(DeviceContext,0,112,debuginfobuffer,(int)strlen(debuginfobuffer));
        Blit_String_To_Buffer(debuginfobuffer,&G_6x7Font,0,80,&color);
        
        
    }
    
    
#endif
    
    HDC DeviceContext = GetDC(G_Window);
    
    StretchDIBits(DeviceContext,0,0,G_Monitorwidth,G_Monitorheigth,0,0,GAME_RES_WIDTH,
                  GAME_RES_HEIGHT,G_BackBuffer.memory,&G_BackBuffer.BitmapInfo,
                  DIB_RGB_COLORS,SRCCOPY);
    
    
    
    
    ReleaseDC(G_Window,DeviceContext);
}

internal void Render_Main_Menu()
{
    __m128i Color = {0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff};;
    
    ClearScreen(Color);
    
}


int CALLBACK WinMain(_In_ HINSTANCE Instance, _In_ HINSTANCE PrevInstance,
                     _In_ LPSTR CmdLine, _In_ int CmdShow)
{
    
    MSG msg = { 0 };
    i64 framestart = 0;
    i64 frameend   = 0;
    i64 ElapsedMSperFrame = 0;
    i64 frameAccumulatorcooked = 0;
    i64 frameAccumulatorraw = 0;
    
    HANDLE ProcessHandle = GetCurrentProcess();
    
    if(LoadRegistry() != ERROR_SUCCESS)
    {
        goto Exit;
    }
    
    FILETIME Createtime = {0};
    FILETIME Exittime   = {0};
    
    HMODULE Ntdllmodule;
    
    if((Ntdllmodule = GetModuleHandleA("ntdll.dll"))== NULL)
    {
        MessageBoxA(NULL,"couldnt load ntd.dll","error",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(Ntdllmodule,"NtQueryTimerResolution"))==NULL)
    {
        MessageBoxA(NULL,"couldnt find NTQueryTimerResolution function ","error",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    
    NtQueryTimerResolution(&G_Performance.MinTimerRes,&G_Performance.MaxTimerRes,
                           &G_Performance.currentTimerRes);
    
    GetSystemInfo(&G_Performance.systemInfo);
    
    if(AlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL,"an instance of this program already on use","error",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    if (timeBeginPeriod(1) == TIMERR_NOCANDO)
    {
        MessageBoxA(NULL,"error !","error iN CHANGING TIMER RESOLUTION ",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    if (SetPriorityClass(ProcessHandle,HIGH_PRIORITY_CLASS) == 0)
    {
        MessageBoxA(NULL,"error !","error in make priority to this process ",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
        
    }
    if(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST) == 0)
    {
        MessageBoxA(NULL,"error !","error in make priority to this thread ",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
        
    }
    
    if(LoadRegistry() != ERROR_SUCCESS)
    {
        goto Exit;
    }
    if (CreateWindowApp()!= ERROR_SUCCESS)
    {
        MessageBoxA(NULL,"error !","error in creating window ",MB_OK|MB_ICONEXCLAMATION);
        goto Exit;
    }
    
    
    QueryPerformanceFrequency((LARGE_INTEGER*)&G_Performance.frequency);
    
    /*HWND sound;
    InitSound(sound);
    */
    
    G_BackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(G_BackBuffer.BitmapInfo.bmiHeader);
    G_BackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
    G_BackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
    G_BackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;
    G_BackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;
    G_BackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;
    G_BackBuffer.memory =VirtualAlloc(NULL, (GAME_RES_WIDTH*GAME_RES_HEIGHT) * GAME_BPP / 8, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    
    
    if((Load32Bitmap("C:\\hosni\\projet\\data\\6x7Font.bmpx",
                     &G_6x7Font)) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL,"load32 6x7font bmp failed !","Error!",MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    
    i64  CurrentUsertime    = 0;
    i64  Currentkerneltime  = 0;
    i64  Previoususertime   = 0;
    i64  Previouskerneltime = 0;
    
    GetSystemTimeAsFileTime((FILETIME*)&G_Performance.PreviousSystemtime);
    
    if (InitPlayer() != ERROR_SUCCESS)
    {
        MessageBoxA(NULL,"Failed to initialize hero","Error",MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    
    G_Performance.displaydebuginfo = TRUE ;
    Pixel32 color= {0} ;
    color.Blue = 255;
    color.Green = 255;
    color.Red = 255;
    color.Alpha = 255;
    while (G_StartGame == FALSE)
    {
        
        Render_Main_Menu();
        Blit_String_To_Buffer("Start new game",&G_6x7Font,190,150,&color);
        Blit_String_To_Buffer("Options",&G_6x7Font,210,160,&color);
        Blit_String_To_Buffer("Exit",&G_6x7Font,220,170,&color);
        HDC DeviceContext = GetDC(G_Window);
        StretchDIBits(DeviceContext,0,0,G_Monitorwidth,G_Monitorheigth,0,0,GAME_RES_WIDTH,
                      GAME_RES_HEIGHT,G_BackBuffer.memory,&G_BackBuffer.BitmapInfo,
                      DIB_RGB_COLORS,SRCCOPY);
        ReleaseDC(G_Window,DeviceContext);
        while (PeekMessageA(&msg,G_Window,0,0,PM_REMOVE))
        {
            DispatchMessage(&msg);
        }
        ProcessInput();
        
        
    }
    while (G_running && G_StartGame)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&framestart);
        while (PeekMessageA(&msg,G_Window,0,0,PM_REMOVE))
        {
            DispatchMessage(&msg);
        }
        ProcessInput();
        RenderGraphics();
        QueryPerformanceCounter((LARGE_INTEGER*)&frameend);
        ElapsedMSperFrame  = frameend - framestart;
        ElapsedMSperFrame *= 1000000;
        ElapsedMSperFrame /= G_Performance.frequency;
        
        
        
        //Sleep(1);
        G_Performance.FramesRendered++;
        frameAccumulatorraw+= ElapsedMSperFrame;
        
        
        while (ElapsedMSperFrame <= TARGET_MICROSECONDS_PER_FRAME )
        {
            
            QueryPerformanceCounter((LARGE_INTEGER*)&frameend);
            ElapsedMSperFrame  = frameend - framestart;
            ElapsedMSperFrame *= 1000000;
            ElapsedMSperFrame /= G_Performance.frequency;
            if (ElapsedMSperFrame <= (TARGET_MICROSECONDS_PER_FRAME * 0.75f))
            {
                Sleep(1);
            }
        }
        frameAccumulatorcooked += ElapsedMSperFrame;
        
        if (G_Performance.FramesRendered % FPS_AVG == 0)
        {
            GetSystemTimeAsFileTime((FILETIME*)&G_Performance.CurrentSystemtime);
            
            GetProcessTimes(ProcessHandle,
                            &Createtime,
                            &Exittime,
                            (FILETIME*)&Currentkerneltime,
                            (FILETIME*)&CurrentUsertime);
            G_Performance.Percent = (Currentkerneltime-Previouskerneltime)+(CurrentUsertime-Previoususertime);
            
            
            G_Performance.Percent /= (G_Performance.CurrentSystemtime - G_Performance.PreviousSystemtime);
            G_Performance.Percent /= G_Performance.systemInfo.dwNumberOfProcessors;
            G_Performance.Percent *= 100;
            
            
            GetProcessHandleCount(ProcessHandle,&G_Performance.HandleCount);
            K32GetProcessMemoryInfo(ProcessHandle,(PROCESS_MEMORY_COUNTERS*)&G_Performance.MemInfo,sizeof(G_Performance.MemInfo));
            
            G_Performance.rawFramesPerSec = 1.0f/((frameAccumulatorraw/FPS_AVG)*0.000001f);
            G_Performance.cookedFramesPerSec = 1.0f/((frameAccumulatorcooked/FPS_AVG)*0.000001f);
            frameAccumulatorraw = 0;
            frameAccumulatorcooked = 0;
            Previoususertime   = CurrentUsertime;
            Previouskerneltime = Currentkerneltime;
            G_Performance.PreviousSystemtime = G_Performance.CurrentSystemtime;
        }
    }
    Exit:
    return (0);
}





