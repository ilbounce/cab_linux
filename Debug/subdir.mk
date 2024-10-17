################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Binance.c \
../Bitfinex.c \
../Detector.c \
../Huobi.c \
../Logger.c \
../Map.c \
../Socket.c \
../Strategy.c \
../Structs.c \
../WebSocket.c \
../base64.c \
../hmac.c \
../http.c \
../mstime.c \
../sha256.c \
../sha384.c 

C_DEPS += \
./Binance.d \
./Bitfinex.d \
./Detector.d \
./Huobi.d \
./Logger.d \
./Map.d \
./Socket.d \
./Strategy.d \
./Structs.d \
./WebSocket.d \
./base64.d \
./hmac.d \
./http.d \
./mstime.d \
./sha256.d \
./sha384.d 

OBJS += \
./Binance.o \
./Bitfinex.o \
./Detector.o \
./Huobi.o \
./Logger.o \
./Map.o \
./Socket.o \
./Strategy.o \
./Structs.o \
./WebSocket.o \
./base64.o \
./hmac.o \
./http.o \
./mstime.o \
./sha256.o \
./sha384.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -Wextra -lmcheck -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean--2e-

clean--2e-:
	-$(RM) ./Binance.d ./Binance.o ./Bitfinex.d ./Bitfinex.o ./Detector.d ./Detector.o ./Huobi.d ./Huobi.o ./Logger.d ./Logger.o ./Map.d ./Map.o ./Socket.d ./Socket.o ./Strategy.d ./Strategy.o ./Structs.d ./Structs.o ./WebSocket.d ./WebSocket.o ./base64.d ./base64.o ./hmac.d ./hmac.o ./http.d ./http.o ./mstime.d ./mstime.o ./sha256.d ./sha256.o ./sha384.d ./sha384.o

.PHONY: clean--2e-

