# 중계 촬영용 빈 시퀀스(길이만 정함)를 만듭니다. 에디터를 끄고 실행:
#   UnrealEditor-Cmd.exe CatchTheTail.uproject -run=pythonscript -script=Tools/CTMakeBroadcastSeq.py
import unreal

EAL = unreal.EditorAssetLibrary
DIR = "/Game/Cinematic"
FPS = 30
for name, sec in (("LS_Broadcast60", 60), ("LS_Broadcast90", 90), ("LS_Broadcast120", 120)):
    path = DIR + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIR, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    seq.set_display_rate(unreal.FrameRate(FPS, 1))
    seq.set_playback_start(0)
    seq.set_playback_end(sec * FPS)
    EAL.save_loaded_asset(seq)
unreal.log("[CT] BROADCAST_SEQ_DONE")
