# Monitor ESP32 — Linux Embarcado (Grau B)

Aplicacao desenvolvida para a disciplina de Sistemas Embarcados — UNISINOS.

## Funcionalidade

Aplicacao em C compilada com Buildroot que roda em Linux embarcado (QEMU x86_64).
Recebe telemetria do ESP32 via UART, grava log persistente e detecta alertas.

## Arquitetura
<img width="1412" height="391" alt="Arquiteutra" src="https://github.com/user-attachments/assets/df0c15df-0ef3-49ee-90d5-f177f3b7e26d" />

## Comunicação entre os sistemas

O ESP32 transmite periodicamente mensagens pela porta serial em formato textual:

STATUS;TEMP:16.8;UMID:75.5;TMAX:20.0;UMAX:75.0;ALERTA:1;LEITURAS:120;TESTE:OFF

A aplicação monitor_esp32 abre a interface serial, realiza o parsing da mensagem e extrai os campos utilizando:

strtok()
strncmp()
atoi()
atof()

## Requisitos atendidos

- Inicializacao automatica no boot via /etc/init.d/S99monitor
- Persistencia de dados em /var/log/monitor_esp32.log
- Comunicacao com o ESP32 via porta serial (/dev/ttyS1 no QEMU)
- Deteccao de alertas com log separado em /var/log/monitor_esp32_alertas.log

## Estrutura

- src/monitor_esp32.c  — codigo fonte da aplicacao
- init/S99monitor      — script de inicializacao automatica
- Config.in            — definicao do pacote para o menuconfig
- monitor_esp32.mk     — regras de compilacao e instalacao no Buildroot
- external.desc        — descricao do pacote externo
- external.mk          — inclusao do pacote no sistema de build

## Como compilar

```bash
cd buildroot/
make BR2_EXTERNAL=/caminho/monitor_esp32 monitor_esp32
make
```

## Como rodar no QEMU com ESP32 conectado

```bash
cd buildroot/output/images/
qemu-system-x86_64 -M pc -kernel bzImage \
  -drive file=rootfs.ext2,if=virtio,format=raw \
  -append "rootwait root=/dev/vda console=ttyS0 noapic" \
  -net nic,model=virtio -net user \
  -serial mon:stdio \
  -chardev serial,id=esp32,path=/dev/ttyUSB0 \
  -device isa-serial,chardev=esp32,index=1 \
  -nographic
```

## Comandos disponiveis (dentro do QEMU)

```bash
# Ver telemetria em tempo real
tail -f /var/log/monitor_esp32.log

# Alterar limite de temperatura
echo "SET_TEMP_MAX=25.0" > /dev/ttyS1

# Alterar limite de umidade
echo "SET_UMID_MAX=80.0" > /dev/ttyS1

# Forcar alerta
echo "TESTE_ALERTA=ON" > /dev/ttyS1

# Resetar limites
echo "RESET_LIMITES" > /dev/ttyS1

# Ver alertas
cat /var/log/monitor_esp32_alertas.log
```

## Hardware

- ESP32-WROOM com firmware FreeRTOS (GA anterior)
- DHT22 — sensor de temperatura e umidade
- Display TM1637
- LED interno de alerta

## Porta serial

O ESP32 é conectado ao sistema Linux através da UART.

Mapeamento:

Ubuntu Host:
/dev/ttyUSB0

Linux emulado:
/dev/ttyS1****

## Autor

Andrio Epping — Engenharia da Computacao UNISINOS — 2026
