#
# SPDX-FileCopyrightText: LineageOS
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from device makefile.
$(call inherit-product, device/fcnt/fuji/device.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

TARGET_BOOT_ANIMATION_RES := 1080

# WitAqua stuff
PROCESSOR_INFO := MediaTek Dimensity 8350 Extreme
WITAQUA_MAINTAINER := kailua

PRODUCT_NAME := lineage_fuji
PRODUCT_DEVICE := fuji
PRODUCT_MANUFACTURER := FCNT
PRODUCT_BRAND := FCNT
PRODUCT_MODEL := M08

PRODUCT_GMS_CLIENTID_BASE := android-motorola

PRODUCT_BUILD_PROP_OVERRIDES += \
    DeviceName=fuji \
    BuildDesc="M08-user 16 W1VHS36H.80-34-2-2-1-5 64c964-a8f54 release-keys MV-324" \
    BuildFingerprint=FCNT/M08/fuji:16/W1VHS36H.80-34-2-2-1-5/64c964-a8f54:user/release-keys
