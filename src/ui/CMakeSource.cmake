set(SCREEN_SRC_DIR   ../Screen)
set(CANVAS_SRC_DIR   ui/canvas)  # branch cmake!!!
set(CONTROL_SRC_DIR  ui/control)
set(WINDOW_SRC_DIR   ui/window)
set(DISPLAY_SRC_DIR  ui/display)

set(SCREEN_SOURCES
    ${SCREEN_SRC_DIR}/Debug.cpp
    ${WINDOW_SRC_DIR}/Init.cpp
    ../Renderer/ProgressBarRenderer.cpp
    ${CONTROL_SRC_DIR}/ProgressBar.cpp
    ${CANVAS_SRC_DIR}/Ramp.cpp
    ${CANVAS_SRC_DIR}/Util.cpp
    ${CANVAS_SRC_DIR}/Icon.cpp
    ${CANVAS_SRC_DIR}/Canvas.cpp
    ${CANVAS_SRC_DIR}/Color.cpp
    ${CANVAS_SRC_DIR}/BufferCanvas.cpp
    ${WINDOW_SRC_DIR}/Window.cpp
    ${WINDOW_SRC_DIR}/ContainerWindow.cpp
    ${WINDOW_SRC_DIR}/SolidContainerWindow.cpp
    ${WINDOW_SRC_DIR}/BufferWindow.cpp
    ${WINDOW_SRC_DIR}/DoubleBufferWindow.cpp
    ${WINDOW_SRC_DIR}/SingleWindow.cpp
)    
set(SCREEN_CUSTOM_SOURCES
        ${WINDOW_SRC_DIR}/custom/DoubleClick.cpp
        ${CANVAS_SRC_DIR}/custom/GeoBitmap.cpp
        ${CANVAS_SRC_DIR}/custom/Pen.cpp
        ${CONTROL_SRC_DIR}/custom/LargeTextWindow.cpp
        ${WINDOW_SRC_DIR}/custom/Window.cpp
        ${WINDOW_SRC_DIR}/custom/WList.cpp
        ${WINDOW_SRC_DIR}/custom/ContainerWindow.cpp
        ${WINDOW_SRC_DIR}/custom/TopWindow.cpp
        ${WINDOW_SRC_DIR}/custom/SingleWindow.cpp
        ${CANVAS_SRC_DIR}/custom/MoreCanvas.cpp
)

set(SCREEN_CUSTOM_SOURCES_IMG "")  # Reset
if (COREGRAPHICS)
    list (APPEND SCREEN_CUSTOM_SOURCES_IMG 
        ${CANVAS_SRC_DIR}/apple/ImageDecoder.cpp)
endif()

if (LIBPNG)
    list (APPEND SCREEN_CUSTOM_SOURCES_IMG 
        ${CANVAS_SRC_DIR}/custom/LibPNG.cpp)
endif()

if (LIBJPEG)
    list (APPEND SCREEN_CUSTOM_SOURCES_IMG 
        ${CANVAS_SRC_DIR}/custom/LibJPEG.cpp)
endif()

if (TIFF)
    list (APPEND SCREEN_CUSTOM_SOURCES_IMG 
        ${CANVAS_SRC_DIR}/custom/LibTiff.cpp)
endif()

if (ENABLE_MESA_KMS)
    list (APPEND SCREEN_SOURCES
        ${CANVAS_SRC_DIR}/egl/GbmSurface.cpp
        ${CANVAS_SRC_DIR}/egl/DrmFrameBuffer.cpp
        ${DISPLAY_SRC_DIR}/egl/DrmDisplay.cpp
        ${DISPLAY_SRC_DIR}/egl/GbmDisplay.cpp
    )
endif()

if(TARGET_IS_ANDROID)  ## ${TARGET} STREQUAL ANDROID)
    list (APPEND SCREEN_SOURCES
        ${SCREEN_CUSTOM_SOURCES}
        ## ${SCREEN_CUSTOM_SOURCES_IMG}
        ${DISPLAY_SRC_DIR}/display/egl/Display.cpp
        ${DISPLAY_SRC_DIR}/egl/ConfigChooser.cpp
        ${CANVAS_SRC_DIR}/egl/TopCanvas.cpp
        ${WINDOW_SRC_DIR}/android/Window.cpp
        ${WINDOW_SRC_DIR}/android/TopWindow.cpp
        ${WINDOW_SRC_DIR}/android/SingleWindow.cpp
        ${CANVAS_SRC_DIR}/android/Bitmap.cpp
        ${CANVAS_SRC_DIR}/android/Font.cpp
    )
    if (TIFF)
        list (APPEND SCREEN_SOURCES 
            ${CANVAS_SRC_DIR}/custom/LibTiff.cpp)
    endif()

endif()

if (DITHER)
    list (APPEND SCREEN_SOURCES
        ${CANVAS_SRC_DIR}/memory/Dither.cpp
    )
endif()

if (FREETYPE)
    list (APPEND SCREEN_SOURCES
        ${CANVAS_SRC_DIR}/freetype/Font.cpp
        ${CANVAS_SRC_DIR}/freetype/Init.cpp
    )
endif()

if (0)  # ?? call bool_or,${APPKIT),${UIKIT))
    list (APPEND SCREEN_SOURCES
        ${CANVAS_SRC_DIR}/apple/Font.cpp)
endif()

if (USE_X11)
    list (APPEND SCREEN_SOURCES
        ${DISPLAY_SRC_DIR}/x11/Display.cpp
        ${WINDOW_SRC_DIR}/x11/TopWindow.cpp
    )
endif()

if (USE_WAYLAND)
    list (APPEND SCREEN_SOURCES
        ${DISPLAY_SRC_DIR}/wayland/Display.cpp
        ${WINDOW_SRC_DIR}/wayland/TopWindow.cpp
    )
endif()

if(ENABLE_OPENGL)
    list(APPEND SCREEN_SOURCES       
        ${DISPLAY_SRC_DIR}/opengl/Display.cpp
        ${CANVAS_SRC_DIR}/custom/Cache.cpp
        ${CANVAS_SRC_DIR}/opengl/Init.cpp
        ${CANVAS_SRC_DIR}/opengl/Dynamic.cpp
        ${CANVAS_SRC_DIR}/opengl/Rotate.cpp
        ${CANVAS_SRC_DIR}/opengl/Geo.cpp
        ${CANVAS_SRC_DIR}/opengl/Globals.cpp
        ${CANVAS_SRC_DIR}/opengl/Extension.cpp
        ${CANVAS_SRC_DIR}/opengl/VertexArray.cpp
        ${CANVAS_SRC_DIR}/opengl/ConstantAlpha.cpp
        ${CANVAS_SRC_DIR}/opengl/Bitmap.cpp
        ${CANVAS_SRC_DIR}/opengl/RawBitmap.cpp
        ${CANVAS_SRC_DIR}/opengl/Canvas.cpp
        ${CANVAS_SRC_DIR}/opengl/BufferCanvas.cpp
        ${CANVAS_SRC_DIR}/opengl/TopCanvas.cpp
        ${CANVAS_SRC_DIR}/opengl/SubCanvas.cpp
        ${CANVAS_SRC_DIR}/opengl/Texture.cpp
        ${CANVAS_SRC_DIR}/opengl/UncompressedImage.cpp
        ${CANVAS_SRC_DIR}/opengl/Buffer.cpp
        ${CANVAS_SRC_DIR}/opengl/Shapes.cpp
        ${CANVAS_SRC_DIR}/opengl/Shaders.cpp
        ${CANVAS_SRC_DIR}/opengl/CanvasRotateShift.cpp
        ${CANVAS_SRC_DIR}/opengl/Triangulate.cpp
  )
 endif()

if (ENABLE_SDL)
    list(APPEND SCREEN_SOURCES       
        ${SCREEN_CUSTOM_SOURCES}
        ${SCREEN_CUSTOM_SOURCES_IMG}
        ${DISPLAY_SRC_DIR}/sdl/Display.cpp
        ${CANVAS_SRC_DIR}/custom/Files.cpp
        ${CANVAS_SRC_DIR}/custom/Bitmap.cpp
        ${CANVAS_SRC_DIR}/custom/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/sdl/TopCanvas.cpp
        ${WINDOW_SRC_DIR}/sdl/Window.cpp
        ${WINDOW_SRC_DIR}/sdl/TopWindow.cpp
        ${WINDOW_SRC_DIR}/sdl/SingleWindow.cpp
    )

    if (NOT OPENGL)
        set(USE_MEMORY_CANVAS ON)
    endif()
elseif (EGL AND NOT TARGET_IS_ANDROID)
    list(APPEND SCREEN_SOURCES       
        ${SCREEN_CUSTOM_SOURCES_IMG}
        ${SCREEN_CUSTOM_SOURCES}
        ${CANVAS_SRC_DIR}/custom/Files.cpp
        ${CANVAS_SRC_DIR}/custom/Bitmap.cpp
        ${CANVAS_SRC_DIR}/custom/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/egl/TopCanvas.cpp
        ${DISPLAY_SRC_DIR}/egl/ConfigChooser.cpp
        ${DISPLAY_SRC_DIR}/egl/Display.cpp
        ${WINDOW_SRC_DIR}/poll/TopWindow.cpp
        ${WINDOW_SRC_DIR}/fb/Window.cpp
        ${WINDOW_SRC_DIR}/fb/SingleWindow.cpp
    )
elseif (GLX)
    list(APPEND SCREEN_SOURCES       
        ${SCREEN_CUSTOM_SOURCES_IMG}
        ${SCREEN_CUSTOM_SOURCES}
        ${CANVAS_SRC_DIR}/custom/Files.cpp
        ${CANVAS_SRC_DIR}/custom/Bitmap.cpp
        ${CANVAS_SRC_DIR}/custom/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/glx/TopCanvas.cpp
        ${WINDOW_SRC_DIR}/poll/TopWindow.cpp
        ${WINDOW_SRC_DIR}/fb/Window.cpp
        ${WINDOW_SRC_DIR}/fb/SingleWindow.cpp
    )
elseif (VFB)
    list(APPEND SCREEN_SOURCES       
        ${SCREEN_CUSTOM_SOURCES_IMG}
        ${SCREEN_CUSTOM_SOURCES}
        ${CANVAS_SRC_DIR}/custom/Files.cpp
        ${CANVAS_SRC_DIR}/custom/Bitmap.cpp
        ${CANVAS_SRC_DIR}/custom/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/fb/TopCanvas.cpp
        ${WINDOW_SRC_DIR}/poll/TopWindow.cpp
        ${WINDOW_SRC_DIR}/fb/Window.cpp
        ${WINDOW_SRC_DIR}/fb/SingleWindow.cpp
    )
    set(FB_CPPFLAGS -DUSE_VFB)
elseif (USE_FB)
    list(APPEND SCREEN_SOURCES       
        ${SCREEN_CUSTOM_SOURCES_IMG}
        ${SCREEN_CUSTOM_SOURCES}
        ${CANVAS_SRC_DIR}/custom/Files.cpp
        ${CANVAS_SRC_DIR}/custom/Bitmap.cpp
        ${CANVAS_SRC_DIR}/custom/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/memory/Export.cpp
        ${WINDOW_SRC_DIR}/poll/TopWindow.cpp
        ${WINDOW_SRC_DIR}/fb/TopWindow.cpp
        ${CANVAS_SRC_DIR}/fb/TopCanvas.cpp
        ${WINDOW_SRC_DIR}/fb/Window.cpp
        ${WINDOW_SRC_DIR}/fb/SingleWindow.cpp
    )
    set(FB_CPPFLAGS -DUSE_FB)
# elseif (HAVE_WIN32)
elseif (WIN32)
    list(APPEND SCREEN_SOURCES
        ${DISPLAY_SRC_DIR}/gdi/Display.cpp
        ${CANVAS_SRC_DIR}/gdi/WindowCanvas.cpp
        ${CANVAS_SRC_DIR}/gdi/VirtualCanvas.cpp
        ${CANVAS_SRC_DIR}/gdi/Font.cpp
        ${WINDOW_SRC_DIR}/gdi/Window.cpp
        ${WINDOW_SRC_DIR}/gdi/PaintWindow.cpp
        ${WINDOW_SRC_DIR}/gdi/ContainerWindow.cpp
        ${CONTROL_SRC_DIR}/gdi/LargeTextWindow.cpp
        ${WINDOW_SRC_DIR}/gdi/SingleWindow.cpp
        ${WINDOW_SRC_DIR}/gdi/TopWindow.cpp
        ${CANVAS_SRC_DIR}/gdi/Pen.cpp
        ${CANVAS_SRC_DIR}/gdi/Brush.cpp
        ${CANVAS_SRC_DIR}/gdi/Bitmap.cpp
        ${CANVAS_SRC_DIR}/gdi/GdiPlusBitmap.cpp
        ${CANVAS_SRC_DIR}/gdi/ResourceBitmap.cpp
        ${CANVAS_SRC_DIR}/gdi/RawBitmap.cpp
        ${CANVAS_SRC_DIR}/gdi/Canvas.cpp
        ${CANVAS_SRC_DIR}/gdi/BufferCanvas.cpp
        ${CANVAS_SRC_DIR}/gdi/PaintCanvas.cpp
    )
    set(GDI_CPPFLAGS -DUSE_GDI)
    set(WINUSER_CPPFLAGS -DUSE_WINUSER)
    set(GDI_LDLIBS -luser32 -lgdi32 -lmsimg32 -lgdiplus)

    # if (TARGET STREQUAL PC)
    if (WIN32)
        list(APPEND GDI_LDLIBS -Wl,-subsystem,windows)
    endif()
endif()

if (TARGET_IS_LINUX)
    list(APPEND SCREEN_SOURCES       
        ${SRC}/ui/linux/GraphicsTTY.cpp)
endif()

if (USE_MEMORY_CANVAS)
    list(APPEND SCREEN_SOURCES       
        ${CANVAS_SRC_DIR}/custom/Cache.cpp
        ${CANVAS_SRC_DIR}/memory/Bitmap.cpp
        ${CANVAS_SRC_DIR}/memory/RawBitmap.cpp
        ${CANVAS_SRC_DIR}/memory/VirtualCanvas.cpp
        ${CANVAS_SRC_DIR}/memory/SubCanvas.cpp
        ${CANVAS_SRC_DIR}/memory/Canvas.cpp
    )
    set(MEMORY_CANVAS_CPPFLAGS -DUSE_MEMORY_CANVAS)
endif()

set(SCREEN_CPPFLAGS_INTERNAL
        ${FREETYPE_CPPFLAGS}
        ${LIBPNG_CPPFLAGS}
        ${LIBJPEG_CPPFLAGS}
        ${LIBTIFF_CPPFLAGS}
        ${COREGRAPHICS_CPPFLAGS}
)

set(SCREEN_CPPFLAGS
        ${LINUX_INPUT_CPPFLAGS}
        ${LIBINPUT_CPPFLAGS}
        ${SDL_CPPFLAGS}
        ${GDI_CPPFLAGS} ${WINUSER_CPPFLAGS}
        ${FREETYPE_FEATURE_CPPFLAGS}
        ${APPKIT_CPPFLAGS}
        ${UIKIT_CPPFLAGS}
        ${MEMORY_CANVAS_CPPFLAGS}
        ${OPENGL_CPPFLAGS}
        ${WAYLAND_CPPFLAGS}
        ${EGL_CPPFLAGS}
        ${EGL_FEATURE_CPPFLAGS}
        ${GLX_CPPFLAGS}
        ${POLL_EVENT_CPPFLAGS}
        ${CONSOLE_CPPFLAGS} ${FB_CPPFLAGS} ${VFB_CPPFLAGS}
)

set(SCREEN_LDLIBS
        ${SDL_LDLIBS}
        ${GDI_LDLIBS}
        ${OPENGL_LDLIBS}
        ${FREETYPE_LDLIBS}
        ${LIBPNG_LDLIBS} ${LIBJPEG_LDLIBS}
        ${LIBTIFF_LDLIBS}
        ${WAYLAND_LDLIBS}
        ${EGL_LDLIBS}
        ${GLX_LDLIBS}
        ${FB_LDLIBS}
        ${COREGRAPHICS_LDLIBS}
        ${APPKIT_LDLIBS}
        ${UIKIT_LDLIBS}
)

## $(eval $(call link-library,screen,SCREEN))

list(APPEND SCREEN_LDADD 
        ${SDL_LDADD}
        ${FB_LDADD}
        ${FREETYPE_LDADD}
        ${LIBPNG_LDADD} ${LIBJPEG_LDADD}
)

##############################################################################################################
include(EventSource.cmake)
set(_SOURCES  
    ${SCREEN_SOURCES}
    ${EVENT_SOURCES}
##   ${SCREEN_CUSTOM_SOURCES}
)
set(SCREEN_HEADERS   
        ${CONTROL_SRC_DIR}/ProgressBar.hpp
# ??        ${CONTROL_SRC_DIR}/ScrollBar.hpp
# ??        ${CONTROL_SRC_DIR}/ScrollBar.cpp
# ??        ${CONTROL_SRC_DIR}/TerminalWindow.hpp
# ??        ${CONTROL_SRC_DIR}/TerminalWindow.cpp
        ${CONTROL_SRC_DIR}/LargeTextWindow.hpp
# ??         ${CONTROL_SRC_DIR}/List.cpp
# ??        ${CONTROL_SRC_DIR}/List.hpp
          
        ${CANVAS_SRC_DIR}/AnyCanvas.hpp
        ${CANVAS_SRC_DIR}/Bitmap.hpp
        ${CANVAS_SRC_DIR}/Brush.hpp
        ${CANVAS_SRC_DIR}/BufferCanvas.cpp
        ${CANVAS_SRC_DIR}/BufferCanvas.hpp
        ${CANVAS_SRC_DIR}/Canvas.cpp
        ${CANVAS_SRC_DIR}/Canvas.hpp
        ${CANVAS_SRC_DIR}/Color.cpp
        ${CANVAS_SRC_DIR}/Color.hpp
        ${CANVAS_SRC_DIR}/Font.hpp  # neu 2021
        ${CANVAS_SRC_DIR}/Icon.cpp
        ${CANVAS_SRC_DIR}/Icon.hpp
        ${CANVAS_SRC_DIR}/Pen.hpp
        ${CANVAS_SRC_DIR}/PortableColor.hpp
        ${CANVAS_SRC_DIR}/Ramp.cpp
        ${CANVAS_SRC_DIR}/Ramp.hpp
        ${CANVAS_SRC_DIR}/RawBitmap.hpp  ### 2021 
        ${CANVAS_SRC_DIR}/SubCanvas.hpp  ### 2021 
        ${CANVAS_SRC_DIR}/Util.cpp
        ${CANVAS_SRC_DIR}/Util.hpp
        ${CANVAS_SRC_DIR}/VirtualCanvas.hpp  ### 2021 
        ${CANVAS_SRC_DIR}/WindowCanvas.hpp  ### 2021 

        ### Window-Sources
        ${WINDOW_SRC_DIR}/BufferWindow.cpp
        ${WINDOW_SRC_DIR}/DoubleBufferWindow.cpp
        ${WINDOW_SRC_DIR}/SingleWindow.cpp
        ${WINDOW_SRC_DIR}/SolidContainerWindow.cpp
        ${WINDOW_SRC_DIR}/Window.cpp
)

set(SCRIPT_FILES 
  CMakeSource.cmake

    ../../build/libevent.mk
    ../../build/screen.mk
    ../../build/test.mk
)


#######################################################################
list(APPEND _SOURCES     ${SCREEN_HEADERS})

# list(APPEND _SOURCES
        # ${EVENT_SRC_DIR}/DelayedNotify.cpp
        # ${EVENT_SRC_DIR}/Globals.cpp
        # ${EVENT_SRC_DIR}/Idle.cpp
        # ${EVENT_SRC_DIR}/Notify.cpp
# )
# if(UNIX)
  # list(APPEND _SOURCES
          # ${EVENT_SRC_DIR}/poll/Timer.cpp
          # ${EVENT_SRC_DIR}/poll/Loop.cpp
          # ${EVENT_SRC_DIR}/poll/Queue.cpp
          # ${EVENT_SRC_DIR}/poll/X11Queue.cpp
  # )
# elseif(WIN32)
  # list(APPEND _SOURCES
          # ${EVENT_SRC_DIR}/shared/Timer.cpp
          # ${EVENT_SRC_DIR}/shared/TimerQueue.cpp

          # ${EVENT_SRC_DIR}/windows/Loop.cpp
          # ${EVENT_SRC_DIR}/windows/Queue.cpp
  # )

  list(APPEND _SOURCES
          ${CONTROL_SRC_DIR}/ScrollBar.cpp
          ${CONTROL_SRC_DIR}/List.cpp
  )
  list(APPEND _SOURCES
          ${CONTROL_SRC_DIR}/TerminalWindow.cpp
  )
# endif()
  # list(APPEND _SOURCES
          # ${EVENT_SRC_DIR}/shared/Timer.cpp
  # )



