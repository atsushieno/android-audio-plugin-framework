package org.androidaudioplugin

class ParameterInformation(var name: String, var content: Int, defaultValue: Float = 0.0f, minimumValue: Float = 0.0f, maximumValue: Float = 1.0f)
{
    var hasValueRange : Boolean = false

    var default: Float = defaultValue
        set(value: Float) {
            hasValueRange = true
            field = value
        }
    var minimum: Float = minimumValue
        set(value: Float) {
            hasValueRange = true
            field = value
        }

    var maximum: Float = maximumValue
        set(value: Float) {
            hasValueRange = true
            field = value
        }
}