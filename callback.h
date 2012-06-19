#ifndef __CALLBACK_H__
    #define __CALLBACK_H__

    #include <common.h>

    struct Block;
    struct Callback
    {
        virtual bool   needTXHash() = 0;
        virtual int          init(int argc, char *argv[]) = 0;

        virtual void     startMap(const uint8_t *p) = 0;
        virtual void       endMap(const uint8_t *p) = 0;
        virtual void   startBlock(const uint8_t *p) = 0;
        virtual void     endBlock(const uint8_t *p) = 0;

        virtual void      startTX(const uint8_t *p) = 0;
        virtual void        endTX(const uint8_t *p) = 0;
        virtual void  startInputs(const uint8_t *p) = 0;
        virtual void    endInputs(const uint8_t *p) = 0;
        virtual void   startInput(const uint8_t *p) = 0;
        virtual void     endInput(const uint8_t *p) = 0;
        virtual void startOutputs(const uint8_t *p) = 0;
        virtual void   endOutputs(const uint8_t *p) = 0;
        virtual void  startOutput(const uint8_t *p) = 0;

        virtual void   startBlock(  const Block *b) = 0;
        virtual void     endBlock(  const Block *b) = 0;

        virtual void endOutput(
            const uint8_t *p,
            uint64_t      value,
            const uint8_t *txHash,
            uint64_t      outputIndex,
            const uint8_t *outputScript,
            uint64_t      outputScriptSize
        ) = 0;

        virtual void edge(
            uint64_t      value,
            const uint8_t *upTXHash,
            uint64_t      outputIndex,
            const uint8_t *outputScript,
            uint64_t      outputScriptSize,
            const uint8_t *downTXHash,
            uint64_t      inputIndex,
            const uint8_t *inputScript,
            uint64_t      inputScriptSize
        ) = 0;

        static void add(const char *name, Callback *callback);
        static Callback *find(const char *name);
    };

#endif // __CALLBACK_H__

