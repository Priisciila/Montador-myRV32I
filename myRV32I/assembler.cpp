#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cstdint>
#include <iomanip>
#include <bitset>

class Instruction {
public:
    std::string label;
    std::string opcode;
    std::vector<std::string> operands;
    
    Instruction() {}
    
    Instruction(std::string label, std::string opcode, std::vector<std::string> operands)
        : label(label), opcode(opcode), operands(operands) {}
        
    void print() const {
        std::cout << "Label: " << (label.empty() ? "(nenhum)" : label) << std::endl;
        std::cout << "Opcode: " << (opcode.empty() ? "(nenhum)" : opcode) << std::endl;
        std::cout << "Operandos: ";
        if (operands.empty()) {
            std::cout << "(nenhum)";
        } else {
            for (size_t i = 0; i < operands.size(); ++i) {
                std::cout << operands[i];
                if (i < operands.size() - 1) {
                    std::cout << ", ";
                }
            }
        }
        std::cout << std::endl;
    }
};

enum InstructionType {
    R_TYPE,
    I_TYPE,
    S_TYPE,
    B_TYPE,
    U_TYPE,
    J_TYPE,
    UNKNOWN
};

class Assembler {
private:
    std::string inputFile;
    std::string outputFile;
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, int> symbolTable;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> registerTable;
    std::unordered_map<std::string, std::pair<InstructionType, std::string>> opcodeTable;
    bool debugMode;  
    
    // Função para inicializar a tabela de registradores
    void initRegisterTable() {
        registerTable["x"] = {};
        for (int i = 0; i < 32; i++) {
            registerTable["x"][std::to_string(i)] = i;
        }
        registerTable["zero"] = {{"", 0}};
        registerTable["ra"] = {{"", 1}};
        registerTable["sp"] = {{"", 2}};
        registerTable["gp"] = {{"", 3}};
        registerTable["tp"] = {{"", 4}};
        
        registerTable["t"] = {};
        for (int i = 0; i <= 2; i++) {
            registerTable["t"][std::to_string(i)] = i + 5;
        }
        for (int i = 3; i <= 6; i++) {
            registerTable["t"][std::to_string(i)] = i + 25;
        }
        
        registerTable["s"] = {};
        registerTable["s"]["0"] = 8;
        registerTable["s"]["1"] = 9;
        for (int i = 2; i <= 11; i++) {
            registerTable["s"][std::to_string(i)] = i + 16;
        }
        
        registerTable["a"] = {};
        for (int i = 0; i <= 7; i++) {
            registerTable["a"][std::to_string(i)] = i + 10;
        }
        
        registerTable["fp"] = {{"", 8}}; 
    }
    
    // Função para inicializar a tabela de opcodes
    void initOpcodeTable() {
        // Instruções tipo R
        opcodeTable["add"] = {R_TYPE, "0110011"};
        opcodeTable["sub"] = {R_TYPE, "0110011"};
        opcodeTable["sll"] = {R_TYPE, "0110011"};
        opcodeTable["slt"] = {R_TYPE, "0110011"};
        opcodeTable["sltu"] = {R_TYPE, "0110011"};
        opcodeTable["xor"] = {R_TYPE, "0110011"};
        opcodeTable["srl"] = {R_TYPE, "0110011"};
        opcodeTable["sra"] = {R_TYPE, "0110011"};
        opcodeTable["or"] = {R_TYPE, "0110011"};
        opcodeTable["and"] = {R_TYPE, "0110011"};
        
        // Instruções M de multiplicação (tipo R)
        opcodeTable["mul"] = {R_TYPE, "0110011"};
        opcodeTable["mulh"] = {R_TYPE, "0110011"};
        opcodeTable["mulhsu"] = {R_TYPE, "0110011"};
        opcodeTable["mulhu"] = {R_TYPE, "0110011"};
        opcodeTable["div"] = {R_TYPE, "0110011"};
        opcodeTable["divu"] = {R_TYPE, "0110011"};
        opcodeTable["rem"] = {R_TYPE, "0110011"};
        opcodeTable["remu"] = {R_TYPE, "0110011"};
        
        // Instruções tipo I
        opcodeTable["addi"] = {I_TYPE, "0010011"};
        opcodeTable["slti"] = {I_TYPE, "0010011"};
        opcodeTable["sltiu"] = {I_TYPE, "0010011"};
        opcodeTable["xori"] = {I_TYPE, "0010011"};
        opcodeTable["ori"] = {I_TYPE, "0010011"};
        opcodeTable["andi"] = {I_TYPE, "0010011"};
        opcodeTable["slli"] = {I_TYPE, "0010011"};
        opcodeTable["srli"] = {I_TYPE, "0010011"};
        opcodeTable["srai"] = {I_TYPE, "0010011"};
        
        // Load (tipo I)
        opcodeTable["lb"] = {I_TYPE, "0000011"};
        opcodeTable["lh"] = {I_TYPE, "0000011"};
        opcodeTable["lw"] = {I_TYPE, "0000011"};
        opcodeTable["lbu"] = {I_TYPE, "0000011"};
        opcodeTable["lhu"] = {I_TYPE, "0000011"};
        
        // Instruções tipo S
        opcodeTable["sb"] = {S_TYPE, "0100011"};
        opcodeTable["sh"] = {S_TYPE, "0100011"};
        opcodeTable["sw"] = {S_TYPE, "0100011"};
        
        // Instruções tipo B
        opcodeTable["beq"] = {B_TYPE, "1100011"};
        opcodeTable["bne"] = {B_TYPE, "1100011"};
        opcodeTable["blt"] = {B_TYPE, "1100011"};
        opcodeTable["bge"] = {B_TYPE, "1100011"};
        opcodeTable["bltu"] = {B_TYPE, "1100011"};
        opcodeTable["bgeu"] = {B_TYPE, "1100011"};
        
        // Instruções tipo U
        opcodeTable["lui"] = {U_TYPE, "0110111"};
        opcodeTable["auipc"] = {U_TYPE, "0010111"};
        
        // Instruções tipo J
        opcodeTable["jal"] = {J_TYPE, "1101111"};
        
        // JALR (tipo I)
        opcodeTable["jalr"] = {I_TYPE, "1100111"};
    }
    
    // Função para converter uma string de registrador para seu número
    int getRegisterNumber(const std::string& reg) {
        if (reg == "zero") return 0;
        
        // Verificar se é x0-x31
        std::regex xreg("x([0-9]+)");
        std::smatch match;
        if (std::regex_match(reg, match, xreg)) {
            int num = std::stoi(match[1]);
            if (num >= 0 && num < 32) {
                return num;
            }
        }
        
        // Verificar outros formatos de registradores (t0, a0, s0, etc.)
        char prefix = reg[0];
        if (reg.size() > 1 && registerTable.find(std::string(1, prefix)) != registerTable.end()) {
            std::string suffix = reg.substr(1);
            if (registerTable[std::string(1, prefix)].find(suffix) != registerTable[std::string(1, prefix)].end()) {
                return registerTable[std::string(1, prefix)][suffix];
            }
        }
        
        if (registerTable.find(reg) != registerTable.end() && registerTable[reg].find("") != registerTable[reg].end()) {
            return registerTable[reg][""];
        }
        
        std::cerr << "Erro: Registrador desconhecido: " << reg << std::endl;
        return -1;
    }
    
    std::string cleanLine(const std::string& line) {
        // Remover comentários
        size_t commentPos = line.find('#');
        std::string cleanedLine = (commentPos != std::string::npos) ? line.substr(0, commentPos) : line;
        
        // Remover espaços extras no início e fim
        cleanedLine = std::regex_replace(cleanedLine, std::regex("^\\s+|\\s+$"), "");
        
        return cleanedLine;
    }
    
    // Função para analisar uma linha de código assembly e lidar com pseudoinstruções
    Instruction parseLine(const std::string& line) {
        std::string cleanedLine = cleanLine(line);
        if (cleanedLine.empty()) {
            return Instruction("", "", {});
        }
        
        std::string label = "";
        std::string opcode = "";
        std::vector<std::string> operands;
        
        // Verificar se há rótulo
        size_t labelPos = cleanedLine.find(':');
        if (labelPos != std::string::npos) {
            label = cleanedLine.substr(0, labelPos);
            label = std::regex_replace(label, std::regex("^\\s+|\\s+$"), "");
            cleanedLine = cleanedLine.substr(labelPos + 1);
            cleanedLine = std::regex_replace(cleanedLine, std::regex("^\\s+"), "");
        }
        
        // Se após remover o rótulo a linha estiver vazia retornar
        if (cleanedLine.empty()) {
            return Instruction(label, "", {});
        }
        
        // Analisar o opcode e operandos
        std::istringstream ss(cleanedLine);
        ss >> opcode;
        
        // Resto da linha são os operandos
        std::string operandsStr;
        std::getline(ss, operandsStr);
        operandsStr = std::regex_replace(operandsStr, std::regex("^\\s+"), "");
        
        // Para instruções de load/store, o formato pode ser "lw rd, offset(rs1)"
        if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "lbu" || opcode == "lhu" ||
            opcode == "sb" || opcode == "sh" || opcode == "sw") {
            
            // Dividir no primeiro operando
            size_t commaPos = operandsStr.find(',');
            if (commaPos != std::string::npos) {
                std::string firstOp = operandsStr.substr(0, commaPos);
                firstOp = std::regex_replace(firstOp, std::regex("^\\s+|\\s+$"), "");
                operands.push_back(firstOp);
                
                std::string secondOp = operandsStr.substr(commaPos + 1);
                secondOp = std::regex_replace(secondOp, std::regex("^\\s+|\\s+$"), "");
                operands.push_back(secondOp);
            }
        } else {
            // Dividir operandos por vírgula
            std::istringstream operandStream(operandsStr);
            std::string operand;
            while (std::getline(operandStream, operand, ',')) {
                operand = std::regex_replace(operand, std::regex("^\\s+|\\s+$"), "");
                if (!operand.empty()) {
                    operands.push_back(operand);
                }
            }
        }
        
        // Lidar com pseudoinstruções
        if (opcode == "j") {
            // "j label" é uma pseudoinstrução para "jal zero, label"
            if (operands.size() == 1) {
                opcode = "jal";
                operands.insert(operands.begin(), "zero");
            }
        } else if (opcode == "jr") {
            // "jr rs" é uma pseudoinstrução para "jalr zero, rs, 0"
            if (operands.size() == 1) {
                opcode = "jalr";
                operands.insert(operands.begin(), "zero");
                operands.push_back("0");
            }
        } else if (opcode == "mv") {
            // "mv rd, rs" é uma pseudoinstrução para "addi rd, rs, 0"
            if (operands.size() == 2) {
                opcode = "addi";
                operands.push_back("0");
            }
        } else if (opcode == "li") {
            // "li rd, imm" é uma pseudoinstrução para "addi rd, zero, imm"
            if (operands.size() == 2) {
                opcode = "addi";
                operands.insert(operands.begin() + 1, "zero");
            }
        } else if (opcode == "nop") {
            // "nop" é uma pseudoinstrução para "addi zero, zero, 0"
            opcode = "addi";
            operands = {"zero", "zero", "0"};
        } else if (opcode == "bgt") {
            // "bgt rs1, rs2, label" é uma pseudoinstrução para "blt rs2, rs1, label"
            if (operands.size() == 3) {
                opcode = "blt";
                std::swap(operands[0], operands[1]);
            }
        } else if (opcode == "ble") {
            // "ble rs1, rs2, label" é uma pseudoinstrução para "bge rs2, rs1, label"
            if (operands.size() == 3) {
                opcode = "bge";
                std::swap(operands[0], operands[1]);
            }
        }
        
        return Instruction(label, opcode, operands);
    }
    
    // Função para codificar instruções tipo R
    std::string encodeRType(const Instruction& instr) {
        // Formato: funct7[31:25] rs2[24:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
        std::string binary = "";
        std::string funct7 = "0000000"; // padrão
        std::string funct3 = "000";     // padrão
        
        // Determinar funct7 com base no opcode
        if (instr.opcode == "sub" || instr.opcode == "sra") {
            funct7 = "0100000";
        } else if (instr.opcode == "mul" || instr.opcode == "mulh" || 
                   instr.opcode == "mulhsu" || instr.opcode == "mulhu" ||
                   instr.opcode == "div" || instr.opcode == "divu" ||
                   instr.opcode == "rem" || instr.opcode == "remu") {
            funct7 = "0000001";  // Extensão M (multiplicação/divisão)
        }
        
        // Determinar funct3 com base no opcode
        if (instr.opcode == "add" || instr.opcode == "sub" || instr.opcode == "mul") funct3 = "000";
        else if (instr.opcode == "sll" || instr.opcode == "mulh") funct3 = "001";
        else if (instr.opcode == "slt" || instr.opcode == "mulhsu") funct3 = "010";
        else if (instr.opcode == "sltu" || instr.opcode == "mulhu") funct3 = "011";
        else if (instr.opcode == "xor" || instr.opcode == "div") funct3 = "100";
        else if (instr.opcode == "srl" || instr.opcode == "sra" || instr.opcode == "divu") funct3 = "101";
        else if (instr.opcode == "or" || instr.opcode == "rem") funct3 = "110";
        else if (instr.opcode == "and" || instr.opcode == "remu") funct3 = "111";
        
        // Obter números dos registradores
        int rd = getRegisterNumber(instr.operands[0]);
        int rs1 = getRegisterNumber(instr.operands[1]);
        int rs2 = getRegisterNumber(instr.operands[2]);
        
        // Construir a instrução binária usando strings
        binary += funct7;
        binary += std::bitset<5>(rs2).to_string();
        binary += std::bitset<5>(rs1).to_string();
        binary += funct3;
        binary += std::bitset<5>(rd).to_string();
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar instruções tipo I
    std::string encodeIType(const Instruction& instr) {
        // Formato: imm[11:0] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
        std::string binary = "";
        std::string funct3 = "000";  // padrão
        
        // Determinar funct3 com base no opcode
        if (instr.opcode == "addi") funct3 = "000";
        else if (instr.opcode == "slti") funct3 = "010";
        else if (instr.opcode == "sltiu") funct3 = "011";
        else if (instr.opcode == "xori") funct3 = "100";
        else if (instr.opcode == "ori") funct3 = "110";
        else if (instr.opcode == "andi") funct3 = "111";
        else if (instr.opcode == "slli") funct3 = "001";
        else if (instr.opcode == "srli" || instr.opcode == "srai") funct3 = "101";
        else if (instr.opcode == "lb") funct3 = "000";
        else if (instr.opcode == "lh") funct3 = "001";
        else if (instr.opcode == "lw") funct3 = "010";
        else if (instr.opcode == "lbu") funct3 = "100";
        else if (instr.opcode == "lhu") funct3 = "101";
        else if (instr.opcode == "jalr") funct3 = "000";
        
        // Obter números dos registradores
        int rd = getRegisterNumber(instr.operands[0]);
        int rs1 = 0;
        int imm = 0;
        
        // Analisar o formato específico da instrução
        if (instr.opcode == "jalr" || 
            instr.opcode == "lb" || instr.opcode == "lh" || instr.opcode == "lw" || 
            instr.opcode == "lbu" || instr.opcode == "lhu") {
            
            // Formato: lw rd, imm(rs1)
            std::string secondOp = instr.operands[1];
            size_t openParen = secondOp.find('(');
            size_t closeParen = secondOp.find(')');
            
            if (openParen != std::string::npos && closeParen != std::string::npos) {
                std::string immStr = secondOp.substr(0, openParen);
                std::string rs1Str = secondOp.substr(openParen + 1, closeParen - openParen - 1);
                
                // Converter para números
                imm = std::stoi(immStr);
                rs1 = getRegisterNumber(rs1Str);
            } else if (instr.opcode == "jalr") {
                // Formatos alternativos para jalr
                if (instr.operands.size() == 2) {
                    // jalr rd, rs1
                    rs1 = getRegisterNumber(instr.operands[1]);
                    imm = 0;
                } else if (instr.operands.size() == 3) {
                    // jalr rd, rs1, imm
                    rs1 = getRegisterNumber(instr.operands[1]);
                    try {
                        imm = std::stoi(instr.operands[2]);
                    } catch (const std::invalid_argument&) {
                        if (symbolTable.find(instr.operands[2]) != symbolTable.end()) {
                            imm = symbolTable[instr.operands[2]];
                        } else {
                            std::cerr << "Erro: Símbolo não encontrado: " << instr.operands[2] << std::endl;
                            return "";
                        }
                    }
                } else {
                    std::cerr << "Erro: Formato inválido para instrução jalr" << std::endl;
                    return "";
                }
            } else {
                std::cerr << "Erro: Formato inválido para instrução de load: " << secondOp << std::endl;
                return "";
            }
        } else {
            // Formato normal: addi rd, rs1, imm
            rs1 = getRegisterNumber(instr.operands[1]);
            
            // Verificar se o imediato é um número ou um símbolo
            try {
                imm = std::stoi(instr.operands[2]);
            } catch (const std::invalid_argument&) {
                // Se não for um número, deve ser um símbolo
                if (symbolTable.find(instr.operands[2]) != symbolTable.end()) {
                    imm = symbolTable[instr.operands[2]];
                } else {
                    std::cerr << "Erro: Símbolo não encontrado: " << instr.operands[2] << std::endl;
                    return "";
                }
            }
        }
        
        // Construir a instrução binária
        std::string immBinary;
        if (instr.opcode == "slli" || instr.opcode == "srli" || instr.opcode == "srai") {
            // Para instruções de shift, os bits do imediato são diferentes
            immBinary = (instr.opcode == "srai" ? "0100000" : "0000000") + std::bitset<5>(imm).to_string();
        } else {
            // Imediato de 12 bits com sinal
            std::string immStr = std::bitset<12>(imm & 0xFFF).to_string();
            immBinary = immStr;
        }
        
        binary += immBinary;
        binary += std::bitset<5>(rs1).to_string();
        binary += funct3;
        binary += std::bitset<5>(rd).to_string();
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar instruções tipo S
    std::string encodeSType(const Instruction& instr) {
        // Formato: imm[11:5] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:0] opcode[6:0]
        std::string binary = "";
        std::string funct3 = "000";  // padrão
        
        // Determinar funct3 com base no opcode
        if (instr.opcode == "sb") funct3 = "000";
        else if (instr.opcode == "sh") funct3 = "001";
        else if (instr.opcode == "sw") funct3 = "010";
        
        // Obter números dos registradores
        int rs2 = getRegisterNumber(instr.operands[0]);
        
        // Formato: sw rs2, imm(rs1)
        std::string secondOp = instr.operands[1];
        size_t openParen = secondOp.find('(');
        size_t closeParen = secondOp.find(')');
        
        int rs1, imm;
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string immStr = secondOp.substr(0, openParen);
            std::string rs1Str = secondOp.substr(openParen + 1, closeParen - openParen - 1);
            
            // Converter para números
            imm = std::stoi(immStr);
            rs1 = getRegisterNumber(rs1Str);
        } else {
            std::cerr << "Erro: Formato inválido para instrução de store: " << secondOp << std::endl;
            return "";
        }
        
        // Construir a instrução binária
        std::string immBinary = std::bitset<12>(imm).to_string();
        
        binary += immBinary.substr(0, 7);  // imm[11:5]
        binary += std::bitset<5>(rs2).to_string();
        binary += std::bitset<5>(rs1).to_string();
        binary += funct3;
        binary += immBinary.substr(7, 5);  // imm[4:0]
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar instruções tipo B
    std::string encodeBType(const Instruction& instr) {
        // Formato: imm[12|10:5] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:1|11] opcode[6:0]
        std::string binary = "";
        std::string funct3 = "000";  // padrão
        
        // Determinar funct3 com base no opcode
        if (instr.opcode == "beq") funct3 = "000";
        else if (instr.opcode == "bne") funct3 = "001";
        else if (instr.opcode == "blt") funct3 = "100";
        else if (instr.opcode == "bge") funct3 = "101";
        else if (instr.opcode == "bltu") funct3 = "110";
        else if (instr.opcode == "bgeu") funct3 = "111";
        
        // Obter números dos registradores
        int rs1 = getRegisterNumber(instr.operands[0]);
        int rs2 = getRegisterNumber(instr.operands[1]);
        
        // Obter o imediato 
        int imm = 0;
        std::string labelName = instr.operands[2];
        if (symbolTable.find(labelName) != symbolTable.end()) {
            // calcula o offset relativo para o branch
            int currentAddress = 0;
            
            // Encontrar o endereço da instrução atual
            for (size_t i = 0; i < instructions.size(); i++) {
                if (&instructions[i] == &instr) {
                    break;
                }
                currentAddress += 4;  // Cada instrução ocupa 4 bytes
            }
            
            int targetAddress = symbolTable[labelName];
            // O offset é relativo ao PC da instrução atual
            imm = targetAddress - currentAddress;
            
            // Para branch, o offset é dividido por 2 
            // e deve ser múltiplo de 2
            if (imm % 2 != 0) {
                std::cerr << "Erro: Offset de branch não é múltiplo de 2: " << imm << std::endl;
                return "";
            }
            imm = imm / 2;
        } else {
            try {
                imm = std::stoi(labelName);
            } catch (const std::invalid_argument&) {
                std::cerr << "Erro: Rótulo não encontrado: " << labelName << std::endl;
                return "";
            }
        }
        
        // Construir a instrução binária como string
        // imm tem 13 bits com sinal (bit 0 é sempre 0)
        std::string immBinary = std::bitset<13>(imm & 0x1FFF).to_string(); // 13 bits
        
        // Reorganizar os bits do imediato conforme o formato B
        binary += immBinary[0];  // imm[12]
        binary += immBinary.substr(2, 6);  // imm[10:5]
        binary += std::bitset<5>(rs2).to_string();
        binary += std::bitset<5>(rs1).to_string();
        binary += funct3;
        binary += immBinary.substr(8, 4);  // imm[4:1]
        binary += immBinary[1];  // imm[11]
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar instruções tipo U
    std::string encodeUType(const Instruction& instr) {
        // Formato: imm[31:12] rd[11:7] opcode[6:0]
        std::string binary = "";
        
        // Obter número do registrador
        int rd = getRegisterNumber(instr.operands[0]);
        
        // Obter o imediato
        int imm;
        try {
            imm = std::stoi(instr.operands[1]);
        } catch (const std::invalid_argument&) {
            // Se não for um número, deve ser um símbolo
            if (symbolTable.find(instr.operands[1]) != symbolTable.end()) {
                imm = symbolTable[instr.operands[1]];
            } else {
                std::cerr << "Erro: Símbolo não encontrado: " << instr.operands[1] << std::endl;
                return "";
            }
        }
        
        // Construir a instrução binária
        std::string immStr = std::bitset<20>(imm).to_string();
        binary += immStr;
        binary += std::bitset<5>(rd).to_string();
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar instruções tipo J
    std::string encodeJType(const Instruction& instr) {
        // Formato: imm[20|10:1|11|19:12] rd[11:7] opcode[6:0]
        std::string binary = "";
        
        // Obter número do registrador
        int rd = getRegisterNumber(instr.operands[0]);
        
        // Obter o imediato 
        int imm = 0;
        
        // Verifica se o segundo operando é um rótulo ou um registrador
        std::string target = instr.operands[1];
        
        if (symbolTable.find(target) != symbolTable.end()) {
            // Calcula o offset relativo para o jump
            int currentAddress = 0;
            
            // Encontrar o endereço da instrução atual
            for (size_t i = 0; i < instructions.size(); i++) {
                if (&instructions[i] == &instr) {
                    break;
                }
                currentAddress += 4;  // Cada instrução ocupa 4 bytes
            }
            
            int targetAddress = symbolTable[target];
            imm = targetAddress - currentAddress;
            
            // Para JAL, o offset é dividido por 2 
            // e deve ser múltiplo de 2
            if (imm % 2 != 0) {
                std::cerr << "Erro: Offset de JAL não é múltiplo de 2: " << imm << std::endl;
                return "";
            }
            imm = imm / 2;
        } else {
            // Se não for um rótulo, tentar interpretar como um número
            try {
                imm = std::stoi(target);
            } catch (const std::invalid_argument&) {
                std::cerr << "Erro: Rótulo não encontrado: " << target << std::endl;
                return "";
            }
        }
        
        // Construir a instrução binária
        // Para JAL, imm tem 21 bits 
        std::string immBinary = std::bitset<21>(imm & 0x1FFFFF).to_string(); // 21 bits
        
        // Reorganizar os bits do imediato conforme o formato J
        binary += immBinary[0];  // imm[20]
        binary += immBinary.substr(10, 10);  // imm[10:1]
        binary += immBinary[9];  // imm[11]
        binary += immBinary.substr(1, 8);  // imm[19:12]
        binary += std::bitset<5>(rd).to_string();
        binary += opcodeTable[instr.opcode].second;
        
        return binary;
    }
    
    // Função para codificar uma instrução para binário
    std::string encodeToBinary(const Instruction& instr) {
        if (instr.opcode.empty()) {
            return "";
        }
        
        // Verificar se o opcode existe na tabela
        if (opcodeTable.find(instr.opcode) == opcodeTable.end()) {
            std::cerr << "Erro: Opcode desconhecido: " << instr.opcode << std::endl;
            return "";
        }
        
        // Verificar se há operandos suficientes
        size_t minOperands = 0;
        InstructionType type = opcodeTable[instr.opcode].first;
        
        switch (type) {
            case R_TYPE:
                minOperands = 3;  // rd, rs1, rs2
                break;
            case I_TYPE:
                minOperands = (instr.opcode == "jalr") ? 2 : 3;  // rd, rs1, imm ou rd, offset
                break;
            case S_TYPE:
                minOperands = 2;  // rs2, offset(rs1)
                break;
            case B_TYPE:
                minOperands = 3;  // rs1, rs2, offset
                break;
            case U_TYPE:
                minOperands = 2;  // rd, imm
                break;
            case J_TYPE:
                minOperands = 2;  // rd, offset
                break;
            default:
                break;
        }
        
        if (instr.operands.size() < minOperands) {
            std::cerr << "Erro: Número insuficiente de operandos para " << instr.opcode 
                      << ". Esperado: " << minOperands << ", Encontrado: " << instr.operands.size() << std::endl;
            return "";
        }
        
        // Codificar de acordo com o tipo da instrução
        std::string binary;
        switch (type) {
            case R_TYPE:
                binary = encodeRType(instr);
                break;
            case I_TYPE:
                binary = encodeIType(instr);
                break;
            case S_TYPE:
                binary = encodeSType(instr);
                break;
            case B_TYPE:
                binary = encodeBType(instr);
                break;
            case U_TYPE:
                binary = encodeUType(instr);
                break;
            case J_TYPE:
                binary = encodeJType(instr);
                break;
            default:
                std::cerr << "Erro: Tipo de instrução desconhecido para " << instr.opcode << std::endl;
                return "";
        }
        
        // Verificar se a codificação foi bem-sucedida
        if (binary.empty()) {
            std::cerr << "Erro: Falha ao codificar a instrução: " << instr.opcode << std::endl;
            return "";
        }
        
        // Verificar se o comprimento do binário é 32 bits
        if (binary.length() != 32) {
            std::cerr << "Erro: Instrução codificada não tem 32 bits: " << binary.length() 
                      << " bits para " << instr.opcode << std::endl;
            return "";
        }
        
        return binary;
    }
    
    // Função para converter binário de 32 bits para formato little-endian binário (4 linhas de 8 bits)
    std::vector<std::string> binaryToLittleEndianBinary(const std::string& binary) {
        std::vector<std::string> result;
        
        if (binary.empty() || binary.length() != 32) {
            return result;
        }
        
        // Dividir em 4 bytes (8 bits cada) em formato little-endian
        // Byte 0 (LSB): bits 7-0
        result.push_back(binary.substr(24, 8));
        // Byte 1: bits 15-8  
        result.push_back(binary.substr(16, 8));
        // Byte 2: bits 23-16
        result.push_back(binary.substr(8, 8));
        // Byte 3 (MSB): bits 31-24
        result.push_back(binary.substr(0, 8));
        
        return result;
    }
    
    // Função para verificar a sintaxe das instruções assembly
    bool validateSyntax() {
        bool isValid = true;
        
        for (size_t i = 0; i < instructions.size(); i++) {
            const Instruction& instr = instructions[i];
            
            // Pular instruções vazias 
            if (instr.opcode.empty()) {
                continue;
            }
            
            // Verificar se o opcode existe
            if (opcodeTable.find(instr.opcode) == opcodeTable.end()) {
                std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Opcode desconhecido '" << instr.opcode << "'" << std::endl;
                isValid = false;
                continue;
            }
            
            // Verifica se há operandos suficientes
            size_t minOperands = 0;
            InstructionType type = opcodeTable[instr.opcode].first;
            
            switch (type) {
                case R_TYPE:
                    minOperands = 3;  // rd, rs1, rs2
                    break;
                case I_TYPE:
                    if (instr.opcode == "jalr") {
                        minOperands = 2;  // rd, rs1[, imm]
                    } else if (instr.opcode.find("l") == 0) {  // Instruções load
                        minOperands = 2;  // rd, offset(rs1)
                    } else {
                        minOperands = 3;  // rd, rs1, imm
                    }
                    break;
                case S_TYPE:
                    minOperands = 2;  // rs2, offset(rs1)
                    break;
                case B_TYPE:
                    minOperands = 3;  // rs1, rs2, offset
                    break;
                case U_TYPE:
                    minOperands = 2;  // rd, imm
                    break;
                case J_TYPE:
                    minOperands = 2;  // rd, offset
                    break;
                default:
                    break;
            }
            
            if (instr.operands.size() < minOperands) {
                std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Número insuficiente de operandos para '" 
                          << instr.opcode << "'. Esperado: " << minOperands 
                          << ", Encontrado: " << instr.operands.size() << std::endl;
                isValid = false;
                continue;
            }
            
            // Verificar registradores válidos
            for (size_t j = 0; j < instr.operands.size(); j++) {
                const std::string& op = instr.operands[j];
                
                // Verifica apenas operandos que devem ser registradores
                bool shouldBeRegister = false;
                
                switch (type) {
                    case R_TYPE:
                        shouldBeRegister = true;  // Todos os operandos são registradores
                        break;
                    case I_TYPE:
                        if (j < 2) shouldBeRegister = true;  // rd, rs1 são registradores
                        break;
                    case S_TYPE:
                        if (j == 0) shouldBeRegister = true;  // rs2 é registrador
                        break;
                    case B_TYPE:
                        if (j < 2) shouldBeRegister = true;  // rs1, rs2 são registradores
                        break;
                    case U_TYPE:
                    case J_TYPE:
                        if (j == 0) shouldBeRegister = true;  // rd é registrador
                        break;
                    default:
                        break;
                }
                
                if (shouldBeRegister) {
                    if (type == S_TYPE && j == 1) {
                        size_t openParen = op.find('(');
                        size_t closeParen = op.find(')');
                        
                        if (openParen != std::string::npos && closeParen != std::string::npos) {
                            std::string rs1Str = op.substr(openParen + 1, closeParen - openParen - 1);
                            if (getRegisterNumber(rs1Str) == -1) {
                                std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Registrador inválido '" 
                                          << rs1Str << "' em '" << op << "'" << std::endl;
                                isValid = false;
                            }
                        } else {
                            std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Formato inválido para instrução de store/load: '" 
                                      << op << "', esperado formato 'offset(rs1)'" << std::endl;
                            isValid = false;
                        }
                    } else {
                        if (getRegisterNumber(op) == -1) {
                            std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Registrador inválido '" << op << "'" << std::endl;
                            isValid = false;
                        }
                    }
                }
            }
            
            // Verificações específicas para tipos de instrução
            if (type == B_TYPE || type == J_TYPE) {
                const std::string& label = instr.operands[instr.operands.size() - 1];
                
                bool isNumber = true;
                try {
                    std::stoi(label);
                } catch (const std::invalid_argument&) {
                    isNumber = false;
                }
                
                if (!isNumber && symbolTable.find(label) == symbolTable.end()) {
                    std::cerr << "Erro de sintaxe na linha " << (i + 1) << ": Rótulo não encontrado '" << label << "'" << std::endl;
                    isValid = false;
                }
            }
        }
        
        return isValid;
    }

public:
    Assembler(const std::string& input, const std::string& output = "memoria.mif")
        : inputFile(input), outputFile(output), debugMode(false) {
    }
    
    void setDebugMode(bool enable) {
        debugMode = enable;
    }
    
    bool firstPass() {
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de entrada: " << inputFile << std::endl;
            return false;
        }
        
        std::string line;
        int address = 0;
        
        while (std::getline(file, line)) {
            Instruction instr = parseLine(line);
            
            // Se a instrução tiver um rótulo, registrar na tabela de símbolos
            if (!instr.label.empty()) {
                symbolTable[instr.label] = address;
            }
            
            // Se a instrução tiver um opcode, incrementar o endereço
            if (!instr.opcode.empty()) {
                instructions.push_back(instr);
                address += 4;  // Cada instrução ocupa 4 bytes
            }
        }
        
        file.close();
        return true;
    }
    
    bool secondPass() {
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de saída: " << outputFile << std::endl;
            return false;
        }
        
        for (size_t i = 0; i < instructions.size(); i++) {
            Instruction& instr = instructions[i];
            
            if (debugMode) {
                std::cout << "Instrução #" << i << " (Endereço: 0x" << std::hex << (i * 4) << std::dec << ")" << std::endl;
                instr.print();
            }
            
            if (instr.opcode.empty()) {
                if (debugMode) {
                    std::cout << "  (Instrução vazia, pulando)" << std::endl;
                }
                continue;
            }
            
            std::string binary = encodeToBinary(instr);
            if (!binary.empty()) {
                std::vector<std::string> binaryBytes = binaryToLittleEndianBinary(binary);
                if (!binaryBytes.empty()) {
                    if (debugMode) {
                        std::cout << "  Código binário: " << binary << std::endl;
                        std::cout << "  Bytes (little-endian):" << std::endl;
                        for (size_t j = 0; j < binaryBytes.size(); j++) {
                            std::cout << "    Byte " << j << ": " << binaryBytes[j] << std::endl;
                        }
                        std::cout << "  Gravando no arquivo de saída" << std::endl;
                        std::cout << std::endl;
                    }
                    
                    // Escrever cada byte em uma linha separada (formato .mif especificado)
                    for (const std::string& byteBinary : binaryBytes) {
                        file << byteBinary << std::endl;
                    }
                } else {
                    std::cerr << "Erro: Falha ao converter instrução para formato little-endian: " << instr.opcode << std::endl;
                    file.close();
                    return false;
                }
            } else {
                std::cerr << "Erro: Falha ao codificar instrução: " << instr.opcode << std::endl;
                file.close();
                return false;
            }
        }
        
        file.close();
        std::cout << "Montagem concluída com sucesso. Arquivo gerado: " << outputFile << std::endl;
        return true;
    }
    
    // Função principal para executar o montador
    bool assemble() {
        initRegisterTable();
        initOpcodeTable();
        
        std::cout << "Iniciando a primeira passagem..." << std::endl;
        if (!firstPass()) {
            return false;
        }
        
        std::cout << "Primeira passagem concluída. Símbolos encontrados: " << symbolTable.size() << std::endl;
        
        if (debugMode) {
            std::cout << "Tabela de símbolos:" << std::endl;
            for (const auto& symbol : symbolTable) {
                std::cout << "  " << symbol.first << " = 0x" << std::hex << symbol.second << std::dec << std::endl;
            }
        }
        
        std::cout << "Validando sintaxe..." << std::endl;
        if (!validateSyntax()) {
            std::cerr << "Erros de sintaxe encontrados. Abortando." << std::endl;
            return false;
        }
        
        std::cout << "Sintaxe válida. Iniciando a segunda passagem..." << std::endl;
        
        if (!secondPass()) {
            return false;
        }
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_entrada.asm> [arquivo_saida.mif] [-d]" << std::endl;
        std::cerr << "  -d: Habilita o modo de depuração (mostra informações detalhadas)" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = "memoria.mif";
    bool debugMode = false;
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d") {
            debugMode = true;
        } else {
            outputFile = arg;
        }
    }
    
    Assembler assembler(inputFile, outputFile);
    
    if (debugMode) {
        std::cout << "Modo de depuração ativado" << std::endl;
        assembler.setDebugMode(true);
    }
    
    if (assembler.assemble()) {
        std::cout << "Montagem concluída com sucesso! Arquivo gerado: " << outputFile << std::endl;
        return 0;
    } else {
        std::cerr << "Erro durante o processo de montagem." << std::endl;
        return 1;
    }
}