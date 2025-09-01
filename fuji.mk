#
# SPDX-FileCopyrightText: LineageOS
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from device makefile.
$(call inherit-product, device/fcnt/fuji/device.mk)

# Inherit some common 2by2 stuff.
$(call inherit-product, vendor/2by2/config/common_full_phone.mk)

TARGET_BOOT_ANIMATION_RES := 1080

PRODUCT_NAME := fuji
PRODUCT_DEVICE := fuji
PRODUCT_MANUFACTURER := FCNT
PRODUCT_BRAND := FCNT
PRODUCT_MODEL := M08

CUSTOM_PROCESSOR_INFO := MediaTek Dimensity 8350 Extreme

PRODUCT_GMS_CLIENTID_BASE := android-fcnt

PRODUCT_BUILD_PROP_OVERRIDES += \
    DeviceName=M08 \
    BuildDesc="M08-user 15 V2VH35.58-32-11 98dcb-acfb2 release-keys MV-324" \
    BuildFingerprint=FCNT/fuji_g_hal/fuji:14/V2VH35.58-32-11/98dcb:user/release-keys
