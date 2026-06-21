#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define LOG_PATH    "/var/log/monitor_esp32.log"
#define ALERTA_PATH "/var/log/monitor_esp32_alertas.log"
#define UART_PATH   "/dev/ttyS1"
#define BUF_SIZE    256

typedef struct {
    float temperatura;
    float umidade;
    float tmax;
    float umax;
    int   alerta;
    int   leituras;
    char  teste[8];
} DadosESP32;

void timestamp(char *buf, size_t len) {
    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

int configurar_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 5;

    return tcsetattr(fd, TCSANOW, &tty);
}

int parsear_status(const char *linha, DadosESP32 *dados) {
    if (strncmp(linha, "STATUS;", 7) != 0) return 0;
    if (strstr(linha, "LEITURA:AGUARDANDO")) return 0;
    if (strstr(linha, "LEITURA:ERRO"))       return 0;

    memset(dados, 0, sizeof(DadosESP32));
    strcpy(dados->teste, "OFF");

    char copia[BUF_SIZE];
    strncpy(copia, linha, BUF_SIZE - 1);

    char *token = strtok(copia, ";");
    while (token != NULL) {
        if (strncmp(token, "TEMP:", 5) == 0)
            dados->temperatura = atof(token + 5);
        else if (strncmp(token, "UMID:", 5) == 0)
            dados->umidade = atof(token + 5);
        else if (strncmp(token, "TMAX:", 5) == 0)
            dados->tmax = atof(token + 5);
        else if (strncmp(token, "UMAX:", 5) == 0)
            dados->umax = atof(token + 5);
        else if (strncmp(token, "ALERTA:", 7) == 0)
            dados->alerta = atoi(token + 7);
        else if (strncmp(token, "LEITURAS:", 9) == 0)
            dados->leituras = atoi(token + 9);
        else if (strncmp(token, "TESTE:", 6) == 0)
            strncpy(dados->teste, token + 6, 7);

        token = strtok(NULL, ";");
    }
    return 1;
}

void gravar_log(const char *caminho, const char *mensagem) {
    FILE *f = fopen(caminho, "a");
    if (!f) return;
    fprintf(f, "%s\n", mensagem);
    fflush(f);
    fclose(f);
}

int main(void) {
    char ts[32];
    char linha[BUF_SIZE];
    char log_buf[512];
    DadosESP32 dados;
    int idx = 0;

    /* Log de inicializacao */
    timestamp(ts, sizeof(ts));
    snprintf(log_buf, sizeof(log_buf), "[%s] monitor_esp32 iniciado", ts);
    gravar_log(LOG_PATH, log_buf);
    printf("%s\n", log_buf);

    /* Abre porta serial */
    int fd = open(UART_PATH, O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", UART_PATH, strerror(errno));
        return 1;
    }

    if (configurar_serial(fd) != 0) {
        fprintf(stderr, "Erro ao configurar serial\n");
        return 1;
    }

    printf("Lendo telemetria de %s...\n", UART_PATH);

    /* Loop principal — le byte a byte e monta linhas */
    char c;
    while (1) {
        int n = read(fd, &c, 1);
        if (n <= 0) continue;

        if (c == '\n' || c == '\r') {
            if (idx > 0) {
                linha[idx] = '\0';
                idx = 0;

                if (!parsear_status(linha, &dados)) continue;

                timestamp(ts, sizeof(ts));

                /* Log normal */
                snprintf(log_buf, sizeof(log_buf),
                    "[%s] TEMP:%.1f UMID:%.1f TMAX:%.1f UMAX:%.1f ALERTA:%d LEITURAS:%d",
                    ts,
                    dados.temperatura,
                    dados.umidade,
                    dados.tmax,
                    dados.umax,
                    dados.alerta,
                    dados.leituras);

                gravar_log(LOG_PATH, log_buf);
                printf("%s\n", log_buf);

                /* Log de alerta */
                if (dados.alerta) {
                    snprintf(log_buf, sizeof(log_buf),
                        "[%s] ALERTA ATIVO - TEMP:%.1f (max %.1f) UMID:%.1f (max %.1f)",
                        ts,
                        dados.temperatura, dados.tmax,
                        dados.umidade,     dados.umax);

                    gravar_log(ALERTA_PATH, log_buf);
                    printf("*** %s\n", log_buf);
                }
            }
        } else if (idx < BUF_SIZE - 1) {
            linha[idx++] = c;
        }
    }

    close(fd);
    return 0;
}
