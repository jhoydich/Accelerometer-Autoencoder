ACCEL_AUTOENCODER_TEST_SRCS := \
tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_autoencoder_test.cc \
tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.cc

ACCEL_AUTOENCODER_TEST_TEST_HDRS := \
tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.h

OUTPUT_HANDLER_TEST_SRCS := \
tensorflow/lite/micro/examples/hello_world/output_handler_test.cc \
tensorflow/lite/micro/examples/hello_world/output_handler.cc

OUTPUT_HANDLER_TEST_HDRS := \
tensorflow/lite/micro/examples/hello_world/output_handler.h \
tensorflow/lite/micro/examples/hello_world/constants.h

ACCEL_AUTOENCODER_SRCS := \
tensorflow/lite/micro/examples/accelerometer_autoencoder/main.cc \
tensorflow/lite/micro/examples/accelerometer_autoencoder/main_functions.cc \
tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.cc \


ACCEL_AUTOENCODER_HDRS := \
tensorflow/lite/micro/examples/accelerometer_autoencoder/accel_model.h \
tensorflow/lite/micro/examples/accelerometer_autoencoder/main_functions.h

#Find any platform - specific rules for this example.
include $(wildcard tensorflow/lite/micro/examples/accelerometer_autoencoder/*/Makefile.inc)

# Tests loading and running the sine model.
$(eval $(call microlite_test,accel_autoencoder_test,\
$(ACCEL_AUTOENCODER_TEST_SRCS),$(ACCEL_AUTOENCODER_TEST_HDRS)))

# Tests producing an output.
$(eval $(call microlite_test,output_handler_test,\
$(OUTPUT_HANDLER_TEST_SRCS),$(OUTPUT_HANDLER_TEST_HDRS)))

# Builds a standalone binary.
$(eval $(call microlite_test,accelerometer_autoencoder,\
$(ACCEL_AUTOENCODER_SRCS),$(ACCEL_AUTOENCODER_HDRS)))
