package org.androidaudioplugin.samples.host.engine

class MidiHelper {
    companion object {

        private fun Int.toBCD() = (this / 10) shl 4 + (this % 10)

        private fun Int.fromBCD() = ((this and 0xF0) shr 4) * 10 + (this and 0xF)

        internal fun toMidiTimeCode(
            frameRate: Int,
            framesPerSeconds: Int,
            frames: Int
        ): Array<UByte> {
            val frameUnit = framesPerSeconds / frameRate
            val seconds = frames / framesPerSeconds
            val ticks = ((frames % framesPerSeconds) / frameUnit)
            return arrayOf(
                ticks.toUByte(),
                (seconds % 60).toBCD().toUByte(),
                (seconds / 60).toBCD().toUByte(),
                (seconds / 3600).toBCD().toUByte()
            )
        }

        internal fun expandSMPTE(frameRate: Int, framesPerSeconds: Int, smpte: Int): Int {
            val ticks = smpte and 0xFF
            val seconds = (smpte shr 8 and 0xFF).fromBCD()
            val minutes = (smpte shr 16 and 0xFF).fromBCD()
            val hours = (smpte shr 24 and 0xFF).fromBCD()
            return (hours * 3600 + minutes * 60 + seconds) * framesPerSeconds + ticks * frameRate
        }

        private fun getNoteSeq(note1: Int, note2: Int, note3: Int) : MutableList<UByte> {
            val noteOnSeq = arrayOf(
                110, 0x90, note1, 0x78, // 1 tick = 100 frames (FRAMES_PER_TICK), 110 ticks = 11000 frames
                0, 0x90, note2, 0x78,
                0, 0x90, note3, 0x78)
                .map {i -> i.toUByte() }
            val noteOffSeq = arrayOf(
                110, 0x80, note1, 0,
                0, 0x80, note2, 0,
                0, 0x80, note3, 0)
                .map {i -> i.toUByte() }
            return noteOnSeq.plus(noteOffSeq).toMutableList()
        }

        internal fun getMidiSequence() : List<UByte> {
            // Maybe we should simply use ktmidi API from fluidsynth-midi-service-j repo ...
            val seq1 = getNoteSeq(0x39, 0x3D, 0x40)
            val seq2 = getNoteSeq(0x3B, 0x3F, 0x42)
            val seq3 = getNoteSeq(0x3D, 0x41, 0x44)
            val seq4 = getNoteSeq(0x3E, 0x42, 0x45)
            val seq5 = getNoteSeq(0x40, 0x44, 0x47)
            val seq6 = getNoteSeq(0x42, 0x46, 0x49)
            val seq7 = getNoteSeq(0x44, 0x48, 0x4B)
            val seq8 = getNoteSeq(0x45, 0x49, 0x4C)
            return seq1 + seq2 + seq3 + seq4 + seq5 + seq5 + seq5 + seq6 + seq7 + seq8
        }

        internal fun groupMidi1EventsByTiming(events: Sequence<List<UByte>>) = sequence {
            val iter = events.iterator()
            var list = mutableListOf<List<UByte>>()
            while (iter.hasNext()) {
                val ev = iter.next()
                if (getFirstMidi1EventDuration(ev) != 0) {
                    if (list.any())
                        yield(list.toList())
                    list = mutableListOf()
                }
                list.add(ev)
            }
            if (list.any())
                yield(list.toList())
        }

        internal fun getFirstMidi1EventDuration(bytes: List<UByte>) : Int {
            var len = 0
            var pos = 0
            var mul = 1
            while (pos < bytes.size) {
                val b = bytes[pos]
                len += (b.toInt() and 0x7F) * mul
                pos++
                if (b < 0x80u)
                    break
                mul *= 0x80
            }
            return len
        }

        internal fun splitMidi1Events(seq: UByteArray) = sequence {
            var cur = 0
            var i = 0
            while (i < seq.size) {
                // delta time
                while (seq[i] >= 0x80u)
                    i++
                i++
                // message
                val evPos = i
                i += if (seq[evPos] == 0xF0u.toUByte())
                    seq.drop(i).indexOf(0xF7u) + 1
                else
                    getFixedMidiEventLength(seq, i)
                yield(seq.slice(IntRange(cur, i - 1)))
                cur = i
            }
        }

        private fun getFixedMidiEventLength(seq: UByteArray, from: Int) =
            when (seq[from].toUInt().toInt() and 0xF0) {
                0xA0, 0xC0 -> 2
                0xF0 ->
                    when (seq[from].toUInt().toInt()) {
                        0xF1, 0xF3 -> 1
                        0xF2 -> 2
                        else -> 0
                    }
                else -> 3
            }
    }
}