
// RIMPEMD-160 test

#include <test.h>
#include <util.h>
#include <rmd160.h>
#include <stdint.h>
#include <string.h>

static bool check(
    bool          ok,
    const uint8_t *rmdHex,
    const uint8_t *dataHex
) {
    uint8_t rmd[kRIPEMD160ByteSize];
    fromHex(rmd, rmdHex, kRIPEMD160ByteSize, false);

    uint8_t data[1024];
    auto dataLen = strlen((const char*)dataHex)/2;
    if(0<dataLen) {
        fromHex(data, dataHex, dataLen, false);
    }

    uint8_t cmp[kRIPEMD160ByteSize];
    rmd160(cmp, data, dataLen);

    TEST_CHECK(
        ok,
        0==memcmp(cmp, rmd, sizeof(rmd)),
        "RIPEMD-160 fails for %s\n",
        dataHex
    );
    return ok;
}

static int test() {

    auto ok = true;
    #define T(x, y) {                       \
        const uint8_t rmdHex[] = x;         \
        const uint8_t dataHex[] = y;        \
        ok = check(ok, rmdHex, dataHex);    \
    }                                       \


        T("9c1185a5c5e9fc54612808977ee8f548b2258d31", "");
        T("c151336c3a443e39174958a0c5d45191c3ef1695", "55");
        T("dae6e695bca1e174cadd2473a51cd47af1eeb2ea", "7cba");
        T("0cd9b2596553a347ef0b76b19dfdd2fc82ab79ea", "4f0344");
        T("3e12da39d1c0c6fd61636b1e2d5cbee5593520a7", "56edf52a");
        T("e6dc6fb08bc890b14ad1a9b76838be6d0d91a7ef", "2fd14ffb71");
        T("828c499b385e5124043756ed0f0e158c50cbb6b7", "00cabaaed8a3");
        T("f08e3dcc9a5e85f01d6a6c9587fa346e004f7eab", "b1e4e106a7dc98");
        T("0337e23d47b24e7725550017c2fd5c9a981a22e2", "ccddcf3f146f1e1b");
        T("2a60e7be7f55d77a6c045cc04d7ecf9386793d84", "27e705ad7bfae3115d");
        T("a31b6c0bb8b396583df8e7033ca720f8f850f925", "c364cd75e689076bf902");
        T("11d989e645eb3b35afbaf669f4565ba5272dbc06", "9dd4506aefe57cfdbebc86");
        T("76b6cde640d22792c36d9dd9f13f8465a890847b", "85770ea2816348571f2fac04");
        T("815d572be48544d004e20107732b0165878b1648", "9ff484003dc220a8704825b388");
        T("2cd79e19c8f20c19956f13d3a8e34906ac997060", "6e2f95217987955d60ec08a5f5c2");
        T("3c88a5d90c6cc64bf5944ab4360e6ed9d509ebe0", "514a8231f357c5618dba971868a17f");
        T("bdadc1a25c2403664cd52c6f0e4619e5c572ae3b", "bf0c47add4a42758d7add4610b7d8992");
        T("37a49f54e851a76464fe9362fcdb3ff6033c7265", "171a4cb594af5a4fc9c7cb39d47405e652");
        T("95e1d2b60e2a08d1e4cafbf35ca249a0f23a5607", "38f62f7c93859eb9d526f00438daea640882");
        T("2eec9e33479373cf29d1bcffc1736cfc14dda837", "4adb13a38862751c139a02f4d304024bf8ca26");
        T("6c7e5e620347d7063231bd4b5fdafd10c16f5705", "09eb70bc4c043d4c8a10e75c68f666f6c0971e4d");
        T("8d1d19625c4e619758840d397fde7c2b75f68ef0", "b5d3adedff45ec14f6ba6358b9ef011b13faeea2f0");
        T("a020e24896cccc1c4208a5111651846b32fb55b7", "a97d9cbe1bba8c29fc97877d591d4c7fc09ae74611d3");
        T("6d7aed7f598b70c86af766894c23455a41dbd255", "0969e7aa199ab108023445c2f2d3e14cc75e60f909ce8f");
        T("07f7642778d7e043ba1a03fd9a6ab11b570e3e21", "ab835fb637a31176e90915602b92bc8a6c68a1e2f94e7b06");
        T("a86975e09018f7d0f908ede71665a60bd445f930", "60980bee2273a2d7f9bd52644438d8dc82fd8d182d287f8145");
        T("f70515fe49e269db69d039ed14d30f6b8b2fcb0d", "32cfb09b8f6d3d27e5717fe97d17bde5e4bb076dbfa036d33de5");
        T("58c90cfa8f0d45f3e756b04583f3482ab94e7bf6", "37413e8302e463e0fa92d6ef7b0ab410b013b635553c2be73654e8");
        T("ea35dc191b185b65ca24c2c72411bf2e0d225063", "b763f812c8a557edd6195afa288a3ea450cbc594362217288ee8bfb0");
        T("23c66ab656cf14d5aa62a3a6577f706eecfd6d49", "623beb472d62e118c8ad23f5e229ffe6069f940ccc9c9a0b02ddf19ea7");

    #undef T
    return ok ? 0 : 1;
}

static SimpleTest t(test,  "ripemd-160");

