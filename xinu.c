#include <xinu.h>
#include <time.h>


sid32 sem_ciclo;         
sid32 sem_luz;            
sid32 sem_prato;          
sid32 mutex_klystron;    
sid32 sem_programacao_futura; 

int32 estado = 0;        
int32 luz_ativa = 0;      
int32 emergencia_ativa = 0; 

// Estrutura para curvas de aquecimento
struct curva_aquecimento {
    char *tipo;
    int32 tempo;   
    int32 potencia;
    char *descricao; 
};

struct curva_aquecimento programas[] = {
    {"Carne", 900, 7, "Aquecimento lento e uniforme para carnes."},
    {"Peixe", 600, 5, "Aquecimento suave para preservar a textura do peixe."},
    {"Frango", 1200, 8, "Aquecimento potente para frango."},
    {"Lasanha", 1500, 6, "Aquecimento balanceado para lasanha."},
    {"Pipoca", 150, 10, "Aquecimento rápido e intenso para pipoca."}
};


void controle_klystron(int32 potencia) {
    wait(mutex_klystron);
    if (potencia < 1 || potencia > 10) {
        kprintf("Potência inválida. Ajustando para 5.\n");
        potencia = 5;
    }
    kprintf("Klystron ajustado para potência: %d\n", potencia);
    signal(mutex_klystron);
}

void bip_finalizacao() {
    int32 i;
    for (i = 0; i < 3; i++) {
        kprintf("\a");
        sleep(1);
    }
    kprintf("Ciclo concluído!\n");
}

void luz_interna(int32 ativa) {
    wait(sem_luz);
    if (ativa && !luz_ativa) {
        kprintf("Luz interna: ON\n");
        luz_ativa = 1;
    } else if (!ativa && luz_ativa) {
        kprintf("Luz interna: OFF\n");
        luz_ativa = 0;
    }
    signal(sem_luz);
}

void rotacao_prato() {
    while (estado == 1) {
        wait(sem_prato);
        if (emergencia_ativa) break;
        kprintf("Prato girando...\n");
        sleep(2);
        signal(sem_prato);
    }
}

void relogio_cortesia() {
    while (1) {
        if (estado == 0 && !emergencia_ativa) { 
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            kprintf("Relógio cortesia: %02d:%02d:%02d\n",
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        }
        sleep(1);
    }
}


void ativar_emergencia() {
    emergencia_ativa = 1;
    estado = 0; 
    kprintf("\n*** EMERGÊNCIA ATIVADA ***\n");
    luz_interna(0); 
}


void selecionar_programa() {
    kprintf("Selecione o programa de cozimento:\n");
    int32 i;
    for (i = 0; i < sizeof(programas) / sizeof(programas[0]); i++) {
        kprintf("%d. %s (%d segundos, Potência %d) - %s\n", i + 1, programas[i].tipo, programas[i].tempo, programas[i].potencia, programas[i].descricao);
    }

    int32 escolha;
    scanf("%d", &escolha);

    if (escolha < 1 || escolha > sizeof(programas) / sizeof(programas[0])) {
        kprintf("Opção inválida.\n");
        return;
    }

    struct curva_aquecimento programa = programas[escolha - 1];
    kprintf("Iniciando %s...\n", programa.tipo);
    iniciar_aquecimento(programa.tempo, programa.potencia);
}


void agendar_acao_futura(int32 tempo) {
    kprintf("Ação agendada para %d segundos.\n", tempo);
    sleep(tempo);
   
    kprintf("Executando ação agendada...\n");
    iniciar_aquecimento(600, 7); /
}


void resfriamento() {
    kprintf("Iniciando resfriamento...\n");
    sleep(5);  
    kprintf("Resfriamento concluído.\n");
}


void iniciar_aquecimento(int32 tempo, int32 potencia) {
    kprintf("Iniciando aquecimento por %d segundos na potência %d...\n", tempo, potencia);

    wait(sem_ciclo);
    luz_interna(1);
    resume(create(rotacao_prato, 4096, 20, "Rotacao", 0));
    controle_klystron(potencia);

    int32 t = 0;
    while (t < tempo) {
        if (emergencia_ativa) break;
        sleep(1);
        t++;
    }

    luz_interna(0);
    signal(sem_ciclo);

    if (!emergencia_ativa) {
        bip_finalizacao();
    } else {
        kprintf("Ciclo interrompido pela emergência.\n");
    }

    // Após o aquecimento, iniciar o resfriamento
    resfriamento();
}


process main(void) {
    sem_ciclo = semcreate(1);
    sem_luz = semcreate(1);
    sem_prato = semcreate(1);
    mutex_klystron = semcreate(1);
    sem_programacao_futura = semcreate(1);

    // Thread para o relógio cortesia
    resume(create(relogio_cortesia, 4096, 20, "Relogio", 0));

    kprintf("Micro-ondas Iniciado\n");
    estado = 0;

    while (1) {
        kprintf("\nMenu:\n");
        kprintf("1. Configurar tempo e potência\n");
        kprintf("2. Selecionar programa\n");
        kprintf("3. Iniciar\n");
        kprintf("4. Pausar\n");
        kprintf("5. Cancelar\n");
        kprintf("6. Ativar emergência\n");
        kprintf("7. Agendar ação futura\n");
        kprintf("8. Sair\n");

        int32 opcao;
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
               
                break;
            case 2:
                selecionar_programa();
                break;
            case 3:
                estado = 1;
                iniciar_aquecimento(900, 7);
                break;
            case 4:
                estado = 2;
                kprintf("Aquecimento pausado.\n");
                break;
            case 5:
                estado = 0;
                kprintf("Aquecimento cancelado.\n");
                break;
            case 6:
                ativar_emergencia();
                break;
            case 7:
                kprintf("Digite o tempo em segundos para agendar ação: ");
                int32 tempo;
                scanf("%d", &tempo);
                agendar_acao_futura(tempo);
                break;
            case 8:
                kprintf("Saindo...\n");
                return 0;
            default:
                kprintf("Opção inválida.\n");
        }
    }

    return 0;
}
