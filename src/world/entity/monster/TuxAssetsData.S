#if defined(__APPLE__)
.section __TEXT,__const
.globl _tux_mc3d_start
.globl _tux_mc3d_end
_tux_mc3d_start:
    .incbin "data/models/tux.mc3d"
_tux_mc3d_end:

.globl _tux_rgba_start
.globl _tux_rgba_end
_tux_rgba_start:
    .incbin "data/models/tux_rgba.bin"
_tux_rgba_end:

.globl _tux_snd_bigjump_start
.globl _tux_snd_bigjump_end
_tux_snd_bigjump_start:
    .incbin "data/sound/tux/bigjump.pcm"
_tux_snd_bigjump_end:

.globl _tux_snd_fall_start
.globl _tux_snd_fall_end
_tux_snd_fall_start:
    .incbin "data/sound/tux/fall.pcm"
_tux_snd_fall_end:

.globl _tux_snd_jump_start
.globl _tux_snd_jump_end
_tux_snd_jump_start:
    .incbin "data/sound/tux/jump.pcm"
_tux_snd_jump_end:

.globl _tux_snd_speech_goodday_start
.globl _tux_snd_speech_goodday_end
_tux_snd_speech_goodday_start:
    .incbin "data/sound/tux/speech_goodday.pcm"
_tux_snd_speech_goodday_end:

.globl _tux_snd_splash_start
.globl _tux_snd_splash_end
_tux_snd_splash_start:
    .incbin "data/sound/tux/splash.pcm"
_tux_snd_splash_end:

.globl _tux_snd_singsagain_start
.globl _tux_snd_singsagain_end
_tux_snd_singsagain_start:
    .incbin "data/sound/tux/singsagain.pcm"
_tux_snd_singsagain_end:

.globl _tux_snd_hq_start
.globl _tux_snd_hq_end
_tux_snd_hq_start:
    .incbin "data/sound/tux/hq.pcm"
_tux_snd_hq_end:

.globl _tux_snd_yeti_gna_start
.globl _tux_snd_yeti_gna_end
_tux_snd_yeti_gna_start:
    .incbin "data/sound/tux/yeti_gna.pcm"
_tux_snd_yeti_gna_end:

.globl _tux_icon_rgba_start
.globl _tux_icon_rgba_end
_tux_icon_rgba_start:
    .incbin "data/images/icon_rgba.bin"
_tux_icon_rgba_end:

.globl _tux_btn_multi_start
.globl _tux_btn_multi_end
_tux_btn_multi_start:
    .incbin "data/images/Sm_multi_rgba.bin"
_tux_btn_multi_end:

.globl _tux_btn_options_start
.globl _tux_btn_options_end
_tux_btn_options_start:
    .incbin "data/images/Sm_options_rgba.bin"
_tux_btn_options_end:

.globl _tux_btn_play_start
.globl _tux_btn_play_end
_tux_btn_play_start:
    .incbin "data/images/Sm_play_rgba.bin"
_tux_btn_play_end:

#else
.section .rodata
.global tux_mc3d_start
.global tux_mc3d_end
tux_mc3d_start:
    .incbin "data/models/tux.mc3d"
tux_mc3d_end:

.global tux_rgba_start
.global tux_rgba_end
tux_rgba_start:
    .incbin "data/models/tux_rgba.bin"
tux_rgba_end:

.global tux_snd_bigjump_start
.global tux_snd_bigjump_end
tux_snd_bigjump_start:
    .incbin "data/sound/tux/bigjump.pcm"
tux_snd_bigjump_end:

.global tux_snd_fall_start
.global tux_snd_fall_end
tux_snd_fall_start:
    .incbin "data/sound/tux/fall.pcm"
tux_snd_fall_end:

.global tux_snd_jump_start
.global tux_snd_jump_end
tux_snd_jump_start:
    .incbin "data/sound/tux/jump.pcm"
tux_snd_jump_end:

.global tux_snd_speech_goodday_start
.global tux_snd_speech_goodday_end
tux_snd_speech_goodday_start:
    .incbin "data/sound/tux/speech_goodday.pcm"
tux_snd_speech_goodday_end:

.global tux_snd_splash_start
.global tux_snd_splash_end
tux_snd_splash_start:
    .incbin "data/sound/tux/splash.pcm"
tux_snd_splash_end:

.global tux_snd_singsagain_start
.global tux_snd_singsagain_end
tux_snd_singsagain_start:
    .incbin "data/sound/tux/singsagain.pcm"
tux_snd_singsagain_end:

.global tux_snd_hq_start
.global tux_snd_hq_end
tux_snd_hq_start:
    .incbin "data/sound/tux/hq.pcm"
tux_snd_hq_end:

.global tux_snd_yeti_gna_start
.global tux_snd_yeti_gna_end
tux_snd_yeti_gna_start:
    .incbin "data/sound/tux/yeti_gna.pcm"
tux_snd_yeti_gna_end:

.global tux_icon_rgba_start
.global tux_icon_rgba_end
tux_icon_rgba_start:
    .incbin "data/images/icon_rgba.bin"
tux_icon_rgba_end:

.global tux_btn_multi_start
.global tux_btn_multi_end
tux_btn_multi_start:
    .incbin "data/images/Sm_multi_rgba.bin"
tux_btn_multi_end:

.global tux_btn_options_start
.global tux_btn_options_end
tux_btn_options_start:
    .incbin "data/images/Sm_options_rgba.bin"
tux_btn_options_end:

.global tux_btn_play_start
.global tux_btn_play_end
tux_btn_play_start:
    .incbin "data/images/Sm_play_rgba.bin"
tux_btn_play_end:

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
#endif
