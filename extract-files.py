#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/fcnt/fuji',
    'hardware/mediatek/libmtkperf_client',
    'hardware/mediatek',
    'hardware/motorola',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    ('vendor.mediatek.hardware.videotelephony@1.0',): lib_fixup_vendor_suffix,
}

blob_fixups: blob_fixups_user_type = {
    'system_ext/priv-app/ImsService/ImsService.apk': blob_fixup()
        .apktool_patch('ims-patches'),
    ('system_ext/etc/init/init.vtservice.rc', 'vendor/etc/init/android.hardware.neuralnetworks-shim-service-mtk.rc'): blob_fixup()
        .regex_replace('start', 'enable'),
    'system_ext/lib64/libimsma.so': blob_fixup()
        .replace_needed('libsink.so', 'libsink-mtk.so'),
    'system_ext/lib64/libsink-mtk.so': blob_fixup()
        .add_needed('libaudioclient_shim.so'),
    'system_ext/lib64/libsource.so': blob_fixup()
        .add_needed('libui_shim.so'),
    'vendor/bin/hw/android.hardware.media.c2@1.2-mediatek-64b': blob_fixup()
        .add_needed('libstagefright_foundation-v33.so')
        .replace_needed('libavservices_minijail_vendor.so', 'libavservices_minijail.so'),
    ('vendor/bin/mnld', 'vendor/lib64/hw/android.hardware.sensors@2.X-subhal-mediatek.so', 'vendor/lib64/mt6897/libcam.utils.sensorprovider.so'): blob_fixup()
        .add_needed('android.hardware.sensors@1.0-convert-shared.so'),
    'vendor/lib64/hw/audio.primary.mediatek.so': blob_fixup()
        .add_needed('libstagefright_foundation-v33.so')
        .replace_needed('libutils.so','libutils-v32.so')
        .replace_needed('libalsautils.so','libalsautils-v31.so')
        .replace_needed('libtinyxml2.so', 'libtinyxml2-v34.so'),
    'vendor/lib64/hw/hwcomposer.mtk_common.so': blob_fixup()
        .add_needed('libprocessgroup_shim.so'),
    ('vendor/lib64/mt6897/libneuralnetworks_sl_driver_mtk_prebuilt.so',
     'vendor/lib64/libstfactory-vendor.so', 'vendor/lib64/libnvram.so',
     'vendor/lib64/libsysenv.so', 'vendor/lib64/libtflite_mtk.so'): blob_fixup()
        .add_needed('libbase_shim.so'),
    ('vendor/lib64/hw/mt6897/android.hardware.camera.provider@2.6-impl-mediatek.so','vendor/lib64/mt6897/libmtkcam_stdutils.so',
     'vendor/lib64/sensors.moto.so'): blob_fixup()
        .replace_needed('libutils.so', 'libutils-v32.so')
        .add_needed('libbase_shim.so'),
    'vendor/etc/vintf/manifest/manifest_media_c2_V1_2_default.xml': blob_fixup()
        .regex_replace('1.1', '1.2')
        .regex_replace('@1.0', '@1.2')
        .regex_replace('default9', 'default'),
    ('vendor/lib64/mt6897/lib3a.flash.so', 'vendor/lib64/mt6897/lib3a.ae.stat.so',
     'vendor/lib64/mt6897/lib3a.sensors.flicker.so', 'vendor/lib64/mt6897/lib3a.sensors.color.so',
     'vendor/lib64/lib3a.ae.pipe.so'): blob_fixup()
        .add_needed('liblog.so'),
    'vendor/lib64/mt6897/libmnl.so': blob_fixup()
        .add_needed('libcutils.so'),
    'vendor/bin/init.oem.fingerprint2.sh': blob_fixup()
        .regex_replace('start \\$\\{hal_list\\[\\$1\\]\\}', 'start vendor.fingerprint-default'),
    'vendor/lib64/mt6897/libmtkcam_hal_aidl_common.so': blob_fixup()
        .replace_needed('android.hardware.camera.common-V2-ndk.so', 'android.hardware.camera.common-V1-ndk.so'),
    'vendor/lib64/vendor.mediatek.hardware.bluetooth.audio-V1-ndk.so': blob_fixup()
        .replace_needed('android.hardware.audio.common-V1-ndk.so', 'android.hardware.audio.common-V2-ndk.so'),
    'vendor/lib64/mt6897/libpqconfig.so': blob_fixup()
        .replace_needed('android.hardware.sensors-V2-ndk.so', 'android.hardware.sensors-V3-ndk.so'),
    ('vendor/lib64/mt6897/lib3a.ae.stat.so',
     'vendor/lib64/libarmnn_ndk.mtk.vndk.so'): blob_fixup()
        .add_needed('liblog.so'),
    ('vendor/bin/hw/mt6897/android.hardware.graphics.allocator-V2-service-mediatek.mt6897',
     'vendor/lib64/egl/mt6897/libGLES_mali.so',
     'vendor/lib64/hw/mt6897/android.hardware.graphics.allocator-V2-mediatek.so',
     'vendor/lib64/hw/mt6897/android.hardware.graphics.mapper@4.0-impl-mediatek.so',
     'vendor/lib64/hw/mt6897/mapper.mediatek.so',
     'vendor/lib64/mt6897/libmtkcam_grallocutils.so',
     'vendor/lib64/libcodec2_fsr.so',
     'vendor/lib64/libmtkcam_grallocutils_aidlv1helper.so',
     'vendor/lib64/vendor.mediatek.hardware.camera.isphal-V1-ndk.so',
     'vendor/lib64/vendor.mediatek.hardware.pq_aidl-V2-ndk.so',
     'vendor/lib64/vendor.mediatek.hardware.pq_aidl-V4-ndk.so',
     'vendor/lib64/vendor.mediatek.hardware.pq_aidl-V7-ndk.so'): blob_fixup()
        .replace_needed('android.hardware.graphics.common-V4-ndk.so', 'android.hardware.graphics.common-V7-ndk.so')
        .replace_needed('android.hardware.graphics.allocator-V1-ndk.so', 'android.hardware.graphics.allocator-V2-ndk.so'),
    'vendor/lib64/mt6897/libneuralnetworks_sl_driver_mtk_prebuilt.so': blob_fixup()
        .clear_symbol_version('AHardwareBuffer_allocate')
        .clear_symbol_version('AHardwareBuffer_createFromHandle')
        .clear_symbol_version('AHardwareBuffer_describe')
        .clear_symbol_version('AHardwareBuffer_getNativeHandle')
        .clear_symbol_version('AHardwareBuffer_lock')
        .clear_symbol_version('AHardwareBuffer_lockPlanes')
        .clear_symbol_version('AHardwareBuffer_release')
        .clear_symbol_version('AHardwareBuffer_unlock'),
    'vendor/lib64/hw/mt6897/vendor.mediatek.hardware.pq_aidl-impl.so': blob_fixup()
        .add_needed('libui_shim.so')
        .replace_needed('libtinyxml2.so', 'libtinyxml2-v34.so'),
    'vendor/etc/init/hw/init.vendor.st21nfc.rc': blob_fixup()
        .regex_replace('libnfc-nci-st-felica.conf', 'libnfc-hal-st-felica.conf'),
    'vendor/etc/init/thermal-mediatek.rc': blob_fixup()
        .regex_replace('android.hardware.thermal-service.mediatek', 'android.hardware.thermal-service.mediatek.fuji'),
    'system_ext/lib64/libgpud_sys.so': blob_fixup()
        .replace_needed('android.hardware.graphics.common-V5-ndk.so', 'android.hardware.graphics.common-V7-ndk.so'),
    ('vendor/lib64/mt6897/libmmlpqImpl.so',
     'vendor/lib64/libpqxmlflagparser.so',
     'vendor/lib64/libpqxmlparser.so',
     'vendor/lib64/librt_extamp_intf.so',
     'vendor/lib64/libsilkybrightnesscore.so'): blob_fixup()
        .replace_needed('libtinyxml2.so', 'libtinyxml2-v34.so'),
    'vendor/bin/hw/android.hardware.media.c2@1.2-mediatek-64b': blob_fixup()
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v34.so')
        .replace_needed('libcodec2_hidl@1.1.so', 'libcodec2_hidl@1.1-v34.so')
        .replace_needed('libcodec2_hidl@1.2.so', 'libcodec2_hidl@1.2-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so'),
    'vendor/lib64/libcodec2_fsr.so': blob_fixup()
        .replace_needed('libui.so', 'libui-v34.so'),
    'vendor/lib64/libcodec2_hidl@1.0-v34.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v35.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libui.so', 'libui-v34.so'),
    'vendor/lib64/libcodec2_hidl@1.1-v34.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v35.so')
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v34.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libui.so', 'libui-v34.so'),
    'vendor/lib64/libcodec2_hidl@1.2-v34.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v35.so')
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v34.so')
        .replace_needed('libcodec2_hidl@1.1.so', 'libcodec2_hidl@1.1-v34.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libui.so', 'libui-v34.so'),
    ('vendor/lib64/libcodec2_hidl_plugin-v34.so',
     'vendor/lib64/libsfplugin_ccodec_utils-v34.so'): blob_fixup()
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so'),
    ('vendor/lib64/libcodec2_mtk_c2store.so',
     'vendor/lib64/libcodec2_mtk_vdec.so',
     'vendor/lib64/libcodec2_mtk_venc.so',
     'vendor/lib64/libcodec2_vpp_fa_plugin.so',
     'vendor/lib64/libcodec2_vpp_mi_plugin.so',
     'vendor/lib64/libcodec2_vpp_qt_plugin.so',
     'vendor/lib64/libcodec2_vpp_rs_plugin.so'): blob_fixup()
        .replace_needed('libcodec2_soft_common.so', 'libcodec2_soft_common-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v34.so')
        .replace_needed('libui.so', 'libui-v34.so'),
    'vendor/lib64/libcodec2_soft_common-v34.so': blob_fixup()
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v34.so')
        .replace_needed('libui.so', 'libui-v34.so'),
    ('vendor/lib64/libcodec2_vpp_AIMEMC_plugin.so',
     'vendor/lib64/libcodec2_vpp_AISR_plugin.so'): blob_fixup()
        .replace_needed('android.hardware.graphics.common-V4-ndk.so', 'android.hardware.graphics.common-V7-ndk.so')
        .replace_needed('libcodec2_soft_common.so', 'libcodec2_soft_common-v34.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v34.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v34.so'),
    'vendor/lib64/libcodec2_vndk-v34.so': blob_fixup()
        .replace_needed('libui.so', 'libui-v34.so')
        .replace_needed('android.hardware.media.bufferpool2-V1-ndk.so', 'android.hardware.media.bufferpool2-V2-ndk.so'),
    ('vendor/bin/hw/android.hardware.security.keymint-service.strongbox-thales',
     'vendor/bin/hw/android.hardware.security.keymint@3.0-service.trustonic',
     'vendor/bin/moto_eSE_tool',
     'vendor/lib64/libjc_keymint-thales.so',
     'vendor/lib64/libtpa.so'): blob_fixup()
        .replace_needed('lib_android_keymaster_keymint_utils.so', 'lib_android_keymaster_keymint_utils-v34.so')
        .replace_needed('libcppbor_external.so', 'libcppbor_external-v34.so')
        .replace_needed('libkeymint.so', 'libkeymint-v34.so'),
    'vendor/lib64/libkeymint-v34.so': blob_fixup()
        .replace_needed('lib_android_keymaster_keymint_utils.so', 'lib_android_keymaster_keymint_utils-v34.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'fuji',
    'fcnt',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    add_firmware_proprietary_file=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
