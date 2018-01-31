/*
 * VirtualPanel.h
 *
 *  Created on: Aug 2, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_VIRTUALPANEL_H
#define PIDP_VIRTUALPANEL_H


#include <cstdint>
#include <string>
#include "CPU.h"
#include "Terminal.h"

namespace pdp8
{

    // User input mode to the console.
    enum ConsoleMode {
        PanelMode,
        CommandMode,
        RunMode,
    };

    class VirtualPanel: public Terminal
    {
    public:
        enum VirtualPanelConstants {
            BufferSize = 256,
        };

        VirtualPanel();
        virtual ~VirtualPanel();

        void processStdin();
        void processWinch(int);


        int vconf(const char * format, va_list args);
        int panelf( int y, int x, const char * format, ... );
        int printw( const char *foramt, ... );

        void updatePanel(uint32_t sx[3]);
        void updatePanel();
        void handleResize();

    protected:
        WINDOW      *vPanel, *console, *command;
        ConsoleMode consoleMode;

        std::shared_ptr<Memory<MAXMEMSIZE, memory_base_t, 12>> M;
        std::shared_ptr<CPU> cpu;

        uint32_t	switches[3];

        std::string     cmdBuffer;
        size_t          cmdCurLoc;
        int             maxx, maxy;

        void updateSwitchRegister(uint32_t o);
        void processPanelMode(int);
        void processCommandMode(int);
        void processRunMode(int);
        void updateCommandDisplay();
        void setCursorLocation();
    };

} /* namespace pdp8 */


#endif //PIDP_VIRTUALPANEL_H
