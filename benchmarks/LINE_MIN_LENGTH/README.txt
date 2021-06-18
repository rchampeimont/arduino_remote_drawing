I tested a few different settings of MIN_LINE_LENGTH in remote_drawing_ux.
This optimization is not used any more, as I have replaced it by a constant sampling period instead of a minimum line length.
Constant sampling period gave better results and gave a predictable bandwidth so was better.
It was better for the user because it allows to draw slowly to be more precise, while min line length forces imprecision.
