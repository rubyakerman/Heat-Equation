import os
import CodeWriter
import sys
import Parser

ASM_EXTENTION = ".asm"
VM_EXTENTION = '.vm'
SLASH_SYMBOL = os.sep

def translateFile(file, code):
    parse = Parser.Parser(file)
    parse.cleaningFile()  # clean comments, etc ..

    while parse.hasMoreCommands() :
        parse.advance()

        pushCommand = parse.commandType() is "C_PUSH"
        popCommand = parse.commandType() is "C_POP"

        if popCommand or pushCommand:
            code.pushPopWriter(parse.commandType(), parse.getArg1(), parse.getArg2())
        else:  # that is an arithmetic command
            code.arithmeticWriter(parse.getCurrentCommand())

"""
checks weather the string ends with directory symbol "/"
and remove it if does.
"""
def removeSlash(string):
    if string.endswith(SLASH_SYMBOL):
        seperatorLength = len(SLASH_SYMBOL)
        string = string[:-seperatorLength]
    return string

"""
creating and initialize a writer object from user input,
assuming the input is directory
"""
def createWriterFromDir(input):
    # removing last slash if exists
    input = removeSlash(input)

    fileName = os.path.basename(input)
    outputFileName = input + os.sep + fileName + ASM_EXTENTION

    return CodeWriter.CodeWriter(outputFileName)

"""
creating and initialize a writer object from user input,
assuming the input is file name
"""
def createWriterFromFileName(input):
    NAME_POS = 0

    outputFileName = input.split('.')[NAME_POS] + ASM_EXTENTION
    return CodeWriter.CodeWriter(outputFileName)

def main(argv):
    input = sys.argv[1]

    if os.path.isdir(input): # that is a directory so translate all VM files included
        writer = createWriterFromDir(input)

        files = os.listdir(input)

        for file in files:
            if file.endswith(VM_EXTENTION):
                #start translating files
                writer.startTranslatingMessage(file)
                translateFile(input + SLASH_SYMBOL + file, writer)

    else: # this is a file name - translate it
        writer = createWriterFromFileName(input)
        # translate and close
        translateFile(input,writer)

    writer.close()

if __name__ == "__main__" :
    main(sys.argv)