//
// Created by atsushi on 19/06/07.
//

#include "AAPNativeBridge.h"
#include <jni.h>
#include <android/binder_ibinder.h>
#include <android/binder_ibinder_jni.h>
#include <android/binder_interface_utils.h>
#include <android/binder_parcel.h>
#include <android/binder_parcel_utils.h>
#include <android/binder_status.h>
#include <android/binder_auto_utils.h>

AIBinder_Class *binder_class;
AIBinder *binder;

const char *interface_descriptor = "androidaudiopluginframework";


void* aap_oncreate(void* args)
{
}

void aap_ondestroy(void* userData)
{
}

binder_status_t aap_ontransact(AIBinder *binder, transaction_code_t code, const AParcel *in, AParcel *out)
{
    return STATUS_OK;
}

jobject Java_org_androidaudiopluginframework_samples_aapbarebonesample(JNIEnv* env)
{
    if(binder == NULL) {
        if (binder_class == NULL)
            binder_class = AIBinder_Class_define(interface_descriptor, aap_oncreate, aap_ondestroy, aap_ontransact);
        binder = AIBinder_new(binder_class, NULL);
    }
    return AIBinder_toJavaBinder(env, binder);
}