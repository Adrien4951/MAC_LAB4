# Compte rendu – Lab 4 : Timers, commande d'un moteur pas à pas

**Cours :** Microcontrôleurs : architecture et communication (ITII – Centrale Nantes)
**Cible :** Nucleo-32 STM32F303K8, moteur pas à pas unipolaire + interface ULN2003
**Binôme :** à compléter

## 1. Objectif

Piloter un moteur pas à pas en générant une séquence de commande sur 4 sorties, à une fréquence réglée par un timer (mode synchronisation, sans attente bloquante), puis faire varier la vitesse avec le potentiomètre et le sens avec le bouton D6.

## 2. Câblage et séquence

| Broche MCU | Entrée ULN2003 | Phase moteur | Bit de `seq` |
|---|---|---|---|
| PA8 | IN1 | A | bit 3 |
| PA7 | IN2 | B | bit 2 |
| PA6 | IN3 | C | bit 1 |
| PA5 | IN4 | D | bit 0 |

Séquence en demi-pas (sens anti-horaire) : `{8, 0xC, 4, 6, 2, 3, 1, 9}`.
Les 4 bits de chaque valeur correspondent directement à PA8..PA5 : il suffit de **décaler la valeur de 5 bits** pour l'écrire sur le port, sans `switch`.

## 3. Configuration des sorties (Q1)

```c
void setup()
{
    pinMode(GPIOB,3,OUTPUT);
    ADCInit();
    pinAnalog(GPIOA,3);

    //pin mode Motor
    pinMode(GPIOA,7,OUTPUT);
    pinMode(GPIOA,8,OUTPUT);
    pinMode(GPIOA,6,OUTPUT);
    pinMode(GPIOA,5,OUTPUT);
    pinMode(GPIOB,1,INPUT_PULLUP);   // bouton D6 : appuyé = 0
}
```

## 4. Un pas avec ODR (Q2)

`digitalWrite` est exclu car les 4 sorties doivent changer en même temps. On efface les bits PA5..PA8 puis on écrit le motif :

```c
void stepCCW(int step){
    static const unsigned char seq[] = {8, 0xc, 4, 6, 2, 3, 1, 9};

    GPIOA->ODR &= ~(0xF << 5);
    GPIOA->ODR |=  (seq[step] << 5);
}
```

`static const` : le tableau reste en flash au lieu d'être recopié sur la pile à chaque appel.
Limite : ODR impose une lecture-modification-écriture (non atomique) ; une interruption modifiant une autre broche de GPIOA entre la lecture et l'écriture serait écrasée.

## 5. Version BSRR (Q4, Q5)

BSRR est en écriture seule : les bits 0–15 mettent une broche à 1, les bits 16–31 la mettent à 0, les bits à 0 n'ont aucun effet. Pas de lecture, donc pas de risque d'écraser une autre broche.

```c
void stepCW(int step){
    static const unsigned char seq[] = {8, 0xc, 4, 6, 2, 3, 1, 9};

    GPIOA->BSRR = 0xF << (5 + 16);   // reset PA5..PA8
    GPIOA->BSRR = seq[step] << 5;    // set des bits du motif
}
```

Le sens de rotation dépend uniquement de l'**ordre de parcours** de la séquence : dans `main`, l'indice `i` est incrémenté pour un sens et décrémenté pour l'autre.

## 6. Choix des paramètres du timer (Q3, Q7)

On utilise **TIM2** (timer 32 bits, sur le bus APB1).

**Horloge du timer.** `startup_clock.c` règle SYSCLK = HCLK = 64 MHz (HSI 8 MHz / 2 × PLL 16). APB1 est divisé par 2 (32 MHz, max 36 MHz), mais quand le prescaler APB1 ≠ 1 l'horloge des timers est doublée : **f_TIM2 = 64 MHz**.

**Formules.**

- fréquence de comptage : `f_cnt = f_TIM2 / (PSC + 1)`
- fréquence de débordement (un pas moteur) : `f_pas = f_cnt / (ARR + 1)`

**Prescaler.** On veut une base de temps simple de 1 kHz (1 tick = 1 ms) :
`PSC = 64000` → `f_cnt = 64 000 000 / 64 001 ≈ 1000 Hz`.
(La valeur exacte serait `PSC = 63999`, l'écart de 0,0016 % est négligeable ici.) PSC est sur 16 bits (max 65535), 64000 rentre.

**Auto-reload.** Avec une base à 1 kHz, `f_pas = 1000 / (ARR + 1)` :

| Cas | ARR | f_pas | Exigence |
|---|---|---|---|
| Lent (potentiomètre au milieu) | 100 | 1000 / 101 ≈ **9,9 Hz** | ≈ 10 Hz (Q3, Q7) |
| Rapide (potentiomètre en butée) | 1 | 1000 / 2 = **500 Hz** | 500 Hz (Q7) |

Ce choix PSC/ARR couvre donc exactement la plage 10 Hz → 500 Hz demandée, avec des valeurs d'ARR entières et simples.

**Loi potentiomètre → ARR.** L'ADC renvoie `val` sur 12 bits (0..4095). On replie la valeur autour du milieu (2047) : la distance au milieu (0..2047) donne la vitesse, le côté donne le sens.

```c
TIM2->ARR = 100 - (val * 99) / 2047;   // val = 0 → 100 (lent), val = 2047 → 1 (rapide)
```

**Préchargement d'ARR (ARPE).** ARR est réécrit à chaque tour de boucle. Sans préchargement, si on écrit un ARR plus petit que la valeur courante de CNT, le compteur ne le recroise jamais et doit aller jusqu'à 2³² (TIM2 est 32 bits) : le moteur se fige. Avec `TIM_CR1_ARPE`, la nouvelle valeur n'est prise en compte qu'au prochain débordement.

```c
void setupTimer(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //enable clock for TIM2
    TIM2->PSC = 64000; //prescaler
    TIM2->ARR = 10; //auto reload value
    TIM2->CR1 |= TIM_CR1_CEN; //enable timer
    TIM2->CNT = 0; //reset counter
    TIM2->CR1 |= TIM_CR1_ARPE;   // ARR mis à jour seulement au débordement
}
```

**Synchronisation non bloquante.** La boucle principale ne fait que *regarder* le drapeau de débordement UIF ; si le timer n'a pas débordé, elle continue (lecture ADC, bouton) :

```c
if ((TIM2->SR & TIM_SR_UIF))
{
    TIM2->SR &= ~TIM_SR_UIF; //clear update event flag
    stepCW(i);
    ...
}
```

## 7. Bouton et sens de rotation (Q6, Q8)

Le bouton D6 (PB1, pull-up, appuyé = 0) est traité par une petite machine à états à deux états (`bp`) pour ne réagir qu'**une fois par appui** (front) et non tant que le bouton est maintenu :

```c
if(((GPIOB->IDR & (1U << 1)) == 0) && bp == false){
    bp = true;
    TIM2->CR1 ^= TIM_CR1_CEN;   // inverse timer enable/disable
}
else if (((GPIOB->IDR & (1U << 1)) != 0) && bp == true){
    bp = false;
}
```

Dans notre version, l'appui **démarre/arrête** le moteur (inversion du bit CEN), et le **sens est donné par le côté du potentiomètre** par rapport au milieu (`turn`). Pour suivre exactement Q8, il suffit d'inverser `turn` à l'appui au lieu de CEN.

## 8. Un tour complet (Q9)

Le moteur fait 64 pas par tour de rotor et possède un réducteur de rapport 64 : un tour de l'axe de sortie = **64 × 64 = 4096 pas**.

```c
void tourne(bool sens)
{
    static int i = 0;
    for (int n = 0; n < 64 * 64; n++)
    {
        while (!(TIM2->SR & TIM_SR_UIF)); // attend le debordement du timer
        TIM2->SR &= ~TIM_SR_UIF;          // clear update event flag
        stepCW(i);
        if (sens) i++; else i--;
        if (i < 0) i = 7;
        if (i > 7) i = 0;
    }
}
```

Durée d'un tour : 4096 / 500 Hz ≈ **8,2 s** à vitesse max, 4096 / 10 Hz ≈ **6 min 50 s** à vitesse min.
Cette fonction attend le timer à chaque pas (elle bloque pendant le tour) et suppose que TIM2 est actif. Pour répondre à Q9, on l'appelle à chaque appui sur D6 en inversant le sens à chaque fois.

## 9. Bilan

- Écrire les 4 phases **en une seule opération** (ODR masqué ou BSRR) est indispensable pour une séquence propre ; BSRR est préférable car atomique.
- Le couple **PSC = 64000 / ARR = 1..100** donne une base de 1 ms et couvre 10–500 Hz avec une formule de conversion simple.
- **ARPE** est nécessaire dès que l'on modifie ARR pendant que le timer compte.
- La lecture non bloquante du drapeau UIF laisse la boucle libre pour lire le potentiomètre et le bouton.
