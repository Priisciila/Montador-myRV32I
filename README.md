# Montador myRV32I

**Universidade Federal do Vale do São Francisco - Engenharia de Computação**

Desenvolvido para fins educacionais em Organização e Arquitetura de Computadores.

## Descrição

O montador traduz código em linguagem assembly para linguagem de máquina, gerando um arquivo binário que representa o mapa de memória utilizável em simuladores. O processo de tradução é realizado em duas etapas:

1. **Primeira etapa**: Localiza todos os rótulos e os registra em uma tabela de símbolos
2. **Segunda etapa**: Traduz cada instrução assembly para código binário de 32 bits

## Arquitetura myRV32I

O processador possui 32 registradores de uso geral e suporta 6 formatos de instrução de 32 bits cada:

- **R-Type**: Operações com registradores
- **I-Type**: Operações imediatas e loads
- **S-Type**: Operações de store
- **B-Type**: Branches condicionais
- **U-Type**: Operações com imediatos superiores
- **J-Type**: Jumps incondicionais

## Como Usar

### Compilação

```bash
g++ -o assembler assembler.cpp
```

### Execução

**Formato básico:**
```bash
./assembler <arquivo_entrada.asm> [arquivo_saida.mif] [-d]
```

**Exemplos:**
```bash
./assembler programa.asm                    # Gera memoria.mif
./assembler programa.asm dump.mif           # Gera dump.mif
./assembler programa.asm dump.mif -d        # Modo debug ativo
```

### Parâmetros

- `arquivo_entrada.asm`: Arquivo de entrada com código assembly (obrigatório)
- `arquivo_saida.mif`: Arquivo de saída com o mapa de memória (opcional, padrão: memoria.mif)
- `-d`: Ativa o modo de depuração com informações detalhadas

## Formato do Arquivo de Entrada

Cada linha pode conter:
```
[rótulo:] [instrução] [# comentário]
```

**Exemplo de código assembly:**
```assembly
main:
    addi a0, zero, 6        # Carrega 6 em a0
    add t1, a0, zero        # Copia a0 para t1
    beq t1, zero, fim       # Se t1 == 0, pula para fim
    addi t1, t1, -1         # Decrementa t1
    j main                  # Volta para main
fim:
    nop                     # Instrução vazia
```

## Formato do Arquivo de Saída

O arquivo `.mif` contém o mapa de memória em formato binário little-endian:

- Cada instrução de 32 bits é dividida em 4 bytes (8 bits cada)
- Cada byte é escrito em uma linha separada
- Formato: LSB primeiro (little-endian)

**Exemplo:**
```
Entrada: addi a0, zero, 6
Saída:
00010011
00000101
01100000
00000000
```

## Instruções Suportadas

### Tipo R (Operações com registradores)
- **Básicas**: `add`, `sub`, `sll`, `slt`, `sltu`, `xor`, `srl`, `sra`, `or`, `and`
- **Extensão M**: `mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu`

### Tipo I (Imediatas e Loads)
- **Aritméticas**: `addi`, `slti`, `sltiu`, `xori`, `ori`, `andi`, `slli`, `srli`, `srai`
- **Loads**: `lb`, `lh`, `lw`, `lbu`, `lhu`
- **Jump**: `jalr`

### Tipo S (Stores)
`sb`, `sh`, `sw`

### Tipo B (Branches)
`beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`

### Tipo U (Upper Immediate)
`lui`, `auipc`

### Tipo J (Jump)
`jal`

## Pseudoinstruções

- `j label` → `jal zero, label`
- `jr rs` → `jalr zero, rs, 0`
- `mv rd, rs` → `addi rd, rs, 0`
- `li rd, imm` → `addi rd, zero, imm`
- `nop` → `addi zero, zero, 0`
- `bgt rs1, rs2, label` → `blt rs2, rs1, label`
- `ble rs1, rs2, label` → `bge rs2, rs1, label`

## Características

- **Sistema de memória**: 4GB (endereçamento de 32 bits)
- **Codificação**: Little-endian
- **Validação de sintaxe**: Verifica registradores, operandos e rótulos
- **Tratamento de erros**: Mensagens detalhadas para depuração
- **Suporte a comentários**: Linhas iniciadas com `#`
- **Rótulos**: Suporte completo para jumps e branches

## Modo Debug

Ative o modo debug com a opção `-d` para ver:

- Tabela de símbolos gerada
- Detalhes de cada instrução processada
- Código binário gerado
- Conversão para formato little-endian

## Limitações

- Instruções devem ter exatamente 32 bits
- Offsets para branches e jumps devem ser múltiplos de 2
- Todos os rótulos devem ser definidos antes do uso
- Não suporta diretivas de montagem (`.data`, `.text`, etc.)
