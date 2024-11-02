#include <windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

global_variable int BytesPerPixel;

internal void
RenderGradient(int XOffset, int YOffset)
{
    int Pitch = BitmapWidth * BytesPerPixel;
    u8 *Row = (u8 *)BitmapMemory;
    
    for (int Y = 0;
         Y < BitmapHeight;
         ++Y)
    {
        u8 *Pixel = (u8 *)Row;
        for(int X = 0;
            X < BitmapWidth;
            ++X)
        {
            // Pixel in memory: BB GG RR XX -> little endian
            *Pixel = (u8)(X + XOffset);
            ++Pixel;
            
            *Pixel = (u8)(Y + YOffset);
            ++Pixel;
            
            *Pixel = (u8)Row;
            ++Pixel;
            
            *Pixel = 0;
            ++Pixel;
        }
        Row += Pitch;
    }
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
        // Optionally, you can check if the result of VirtualFree is not zero.
        // Print out an error message if it is.
    }
    
    BitmapWidth = Width;
    BitmapHeight = Height;
    
    //BITMAPINFO BitmapInfo = {}; TODO: check why initialization makes it won't work
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // negative value: top-down pitch
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    BytesPerPixel = 4;
    int BitmapMemorySize = BytesPerPixel * (BitmapWidth * BitmapHeight);
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);   
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect)
{
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    
    StretchDIBits(DeviceContext, 
                  0, 0, WindowWidth, WindowHeight, // destination rectangle (window)
                  0, 0, BitmapWidth, BitmapHeight, // source rectangle (bitmap buffer)
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);  
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, 
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;
        
        case WM_DESTROY:
        {
            Running = false;
        } break;
        
        case WM_CLOSE:
        {
            Running = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;

            HDC DeviceContext = BeginPaint(Window, &Paint);
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Win32UpdateWindow(DeviceContext, &ClientRect);
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return (Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hCursor;
    // WindowClass.hIcon; 
    WindowClass.lpszClassName = "Win32WindowClass";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                            0,
                            WindowClass.lpszClassName,
                            "Win32 Window",
                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            0,
                            0,
                            Instance,
                            0);
        if(Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            Running = true;
            while(Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                
                RenderGradient(XOffset, YOffset);
                ++XOffset;

                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                Win32UpdateWindow(DeviceContext, &ClientRect);
                ReleaseDC(Window, DeviceContext);
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }
    
    return(0);
}