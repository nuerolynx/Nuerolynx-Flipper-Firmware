#include "nlx_tool_registry.h"

#include <string.h>

static const NlxToolDefinition nlx_tools[NlxToolCount] = {
    [NlxToolBuiltInNfc] = {NlxToolBuiltInNfc, "Built-in NFC", "NFC", false, false},
    [NlxToolBuiltInLfRfid] = {NlxToolBuiltInLfRfid, "125 kHz RFID", "125 kHz RFID", false, false},
    [NlxToolBuiltInIbutton] = {NlxToolBuiltInIbutton, "iButton", "iButton", false, false},
    [NlxToolPicopass] =
        {NlxToolPicopass, "Picopass", "/ext/apps/NFC/Credentials/picopass.fap", false, false},
    [NlxToolMfcEditor] =
        {NlxToolMfcEditor,
         "MFC Editor",
         "/ext/apps/NFC/Credentials/mfc_editor.fap",
         false,
         false},
    [NlxToolMfkey] = {NlxToolMfkey, "Mfkey", "/ext/apps/NFC/Lab/mfkey.fap", true, false},
    [NlxToolNfcApduRunner] =
        {NlxToolNfcApduRunner,
         "NFC APDU Runner",
         "/ext/apps/NFC/Lab/nfc_apdu_runner.fap",
         true,
         false},
    [NlxToolNfcMagic] =
        {NlxToolNfcMagic, "NFC Magic", "/ext/apps/NFC/Lab/nfc_magic.fap", true, false},
    [NlxToolMetroflip] =
        {NlxToolMetroflip, "Metroflip", "/ext/apps/NFC/Credentials/metroflip.fap", false, false},
    [NlxToolSeader] =
        {NlxToolSeader, "Seader", "/ext/apps/NFC/Credentials/seader.fap", true, true},
    [NlxToolSeos] =
        {NlxToolSeos, "SEOS", "/ext/apps/NFC/Credentials/seos.fap", true, true},
    [NlxToolIso15693Writer] =
        {NlxToolIso15693Writer,
         "ISO15693 Writer",
         "/ext/apps/NFC/Lab/iso15693_nfc_writer.fap",
         true,
         false},
    [NlxToolFieldDetector] =
        {NlxToolFieldDetector,
         "NFC/RFID Field Detector",
         "/ext/apps/Tools/Security/nfc_rfid_detector.fap",
         false,
         false},
    [NlxToolRfidFuzzer] =
        {NlxToolRfidFuzzer, "RFID Fuzzer", "/ext/apps/RFID/fuzzer_rfid.fap", true, false},
    [NlxToolMifareFuzzer] =
        {NlxToolMifareFuzzer,
         "MIFARE Fuzzer",
         "/ext/apps/NFC/Lab/mifare_fuzzer.fap",
         true,
         false},
    [NlxToolIbuttonFuzzer] =
        {NlxToolIbuttonFuzzer,
         "iButton Fuzzer",
         "/ext/apps/iButton/fuzzer_ibtn.fap",
         true,
         false},
};

const NlxToolDefinition* nlx_tool_registry_get(NlxToolId id) {
    return id < NlxToolCount ? &nlx_tools[id] : NULL;
}

NlxToolId nlx_tool_registry_recommend(const NlxCredentialResult* result) {
    if(!result) return NlxToolBuiltInNfc;
    switch(result->technology) {
    case NlxTechnologyLfRfid:
        return NlxToolBuiltInLfRfid;
    case NlxTechnologyPicopass:
        return NlxToolPicopass;
    case NlxTechnologyIbutton:
        return NlxToolBuiltInIbutton;
    case NlxTechnologyNfc:
        if(strstr(result->card_family, "MIFARE Classic")) return NlxToolMfcEditor;
        return NlxToolBuiltInNfc;
    default:
        return NlxToolBuiltInNfc;
    }
}
