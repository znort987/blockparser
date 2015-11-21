#ifndef __OPCODE_H__
    #define __OPCODE_H__

    #include <common.h>

    #define OPCODES                     \
        OPCODE(                  0, 00) \
        OPCODE(          PUSHDATA1, 4c) \
        OPCODE(          PUSHDATA2, 4d) \
        OPCODE(          PUSHDATA4, 4e) \
        OPCODE(            1NEGATE, 4f) \
        OPCODE(           RESERVED, 50) \
        OPCODE(                  1, 51) \
        OPCODE(                  2, 52) \
        OPCODE(                  3, 53) \
        OPCODE(                  4, 54) \
        OPCODE(                  5, 55) \
        OPCODE(                  6, 56) \
        OPCODE(                  7, 57) \
        OPCODE(                  8, 58) \
        OPCODE(                  9, 59) \
        OPCODE(                 10, 5a) \
        OPCODE(                 11, 5b) \
        OPCODE(                 12, 5c) \
        OPCODE(                 13, 5d) \
        OPCODE(                 14, 5e) \
        OPCODE(                 15, 5f) \
        OPCODE(                 16, 60) \
        OPCODE(                NOP, 61) \
        OPCODE(                VER, 62) \
        OPCODE(                 IF, 63) \
        OPCODE(              NOTIF, 64) \
        OPCODE(              VERIF, 65) \
        OPCODE(           VERNOTIF, 66) \
        OPCODE(               ELSE, 67) \
        OPCODE(              ENDIF, 68) \
        OPCODE(             VERIFY, 69) \
        OPCODE(             RETURN, 6a) \
        OPCODE(         TOALTSTACK, 6b) \
        OPCODE(       FROMALTSTACK, 6c) \
        OPCODE(              2DROP, 6d) \
        OPCODE(               2DUP, 6e) \
        OPCODE(               3DUP, 6f) \
        OPCODE(              2OVER, 70) \
        OPCODE(               2ROT, 71) \
        OPCODE(              2SWAP, 72) \
        OPCODE(              IFDUP, 73) \
        OPCODE(              DEPTH, 74) \
        OPCODE(               DROP, 75) \
        OPCODE(                DUP, 76) \
        OPCODE(                NIP, 77) \
        OPCODE(               OVER, 78) \
        OPCODE(               PICK, 79) \
        OPCODE(               ROLL, 7a) \
        OPCODE(                ROT, 7b) \
        OPCODE(               SWAP, 7c) \
        OPCODE(               TUCK, 7d) \
        OPCODE(                CAT, 7e) \
        OPCODE(             SUBSTR, 7f) \
        OPCODE(               LEFT, 80) \
        OPCODE(              RIGHT, 81) \
        OPCODE(               SIZE, 82) \
        OPCODE(             INVERT, 83) \
        OPCODE(                AND, 84) \
        OPCODE(                 OR, 85) \
        OPCODE(                XOR, 86) \
        OPCODE(              EQUAL, 87) \
        OPCODE(        EQUALVERIFY, 88) \
        OPCODE(          RESERVED1, 89) \
        OPCODE(          RESERVED2, 8a) \
        OPCODE(               1ADD, 8b) \
        OPCODE(               1SUB, 8c) \
        OPCODE(               2MUL, 8d) \
        OPCODE(               2DIV, 8e) \
        OPCODE(             NEGATE, 8f) \
        OPCODE(                ABS, 90) \
        OPCODE(                NOT, 91) \
        OPCODE(          0NOTEQUAL, 92) \
        OPCODE(                ADD, 93) \
        OPCODE(                SUB, 94) \
        OPCODE(                MUL, 95) \
        OPCODE(                DIV, 96) \
        OPCODE(                MOD, 97) \
        OPCODE(             LSHIFT, 98) \
        OPCODE(             RSHIFT, 99) \
        OPCODE(            BOOLAND, 9a) \
        OPCODE(             BOOLOR, 9b) \
        OPCODE(           NUMEQUAL, 9c) \
        OPCODE(     NUMEQUALVERIFY, 9d) \
        OPCODE(        NUMNOTEQUAL, 9e) \
        OPCODE(           LESSTHAN, 9f) \
        OPCODE(        GREATERTHAN, a0) \
        OPCODE(    LESSTHANOREQUAL, a1) \
        OPCODE( GREATERTHANOREQUAL, a2) \
        OPCODE(                MIN, a3) \
        OPCODE(                MAX, a4) \
        OPCODE(             WITHIN, a5) \
        OPCODE(          RIPEMD160, a6) \
        OPCODE(               SHA1, a7) \
        OPCODE(             SHA256, a8) \
        OPCODE(            HASH160, a9) \
        OPCODE(            HASH256, aa) \
        OPCODE(      CODESEPARATOR, ab) \
        OPCODE(           CHECKSIG, ac) \
        OPCODE(     CHECKSIGVERIFY, ad) \
        OPCODE(      CHECKMULTISIG, ae) \
        OPCODE(CHECKMULTISIGVERIFY, af) \
        OPCODE(               NOP1, b0) \
        OPCODE(               NOP2, b1) \
        OPCODE(               NOP3, b2) \
        OPCODE(               NOP4, b3) \
        OPCODE(               NOP5, b4) \
        OPCODE(               NOP6, b5) \
        OPCODE(               NOP7, b6) \
        OPCODE(               NOP8, b7) \
        OPCODE(               NOP9, b8) \
        OPCODE(              NOP10, b9) \
        OPCODE(       SMALLINTEGER, fa) \
        OPCODE(            PUBKEYS, fb) \
        OPCODE(         PUBKEYHASH, fd) \
        OPCODE(             PUBKEY, fe) \
        OPCODE(      INVALIDOPCODE, ff) \

    #define OPCODE(x, y) kOP_##x = 0x##y,
        enum Opcode {
            OPCODES
            kOP_BOGUS_TO_GET_GCC_TO_STFU
        };
    #undef OPCODE

    const char *getOpcodeName(uint8_t op);

#endif // __OPCODE_H__

