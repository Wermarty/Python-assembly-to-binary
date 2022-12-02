#! /usr/bin/env python3
# coding: utf-8

#############################
# Script python pour automatiser les tests d'intégration d'un un programme exécutable,
# en comparant :
# * le code erreur (valeur de retour du programme testé)
# * la sortie dans le terminal (stdout et stderr du programme testé)
# a un oracle.
# Optionnellment :
# * vérifie l'absence d'erreurs en lancant le programme dans valgrind sur chaque test
# Le script utilise comme oracle des fichier .test
#   et génère des fichiers .test en sortie.
#############################

#############################
#############################

# import importlib
# pexpect_spec = importlib.util.find_spec("pexpect")
# if pexpect_spec is None :
#     print("package pexpect was not found.")
#     print("You may want to install it by running:")
#     print("    " +  "pip3 install pexpect --user")
#     sys.exit(1)
try:
    import pexpect
except ImportError or ModuleNotFoundError:
    print("The python package   pexpect   was not found.")
    print("You can install it by running:")
    print("    " +  "pip3 install pexpect --user")
    sys.exit(1)

try:
    import termcolor
except ImportError or ModuleNotFoundError:
    print("The python package   termcolor   was not found.")
    print("You can install it by running:")
    print("    " +  "pip3 install termcolor --user")
    sys.exit(1)

import glob
import os
import re
import sys
import subprocess
import shlex
import signal
import time
import distutils
import shutil
import pexpect
import termcolor
import json
import argparse, textwrap


#############
# various useful global variables
#############
_sleep_between_tests = 0.05




##############
# Parsing of command-line arguments
##############

# from argparse import RawTextHelpFormatter

def parseScriptArgs():
    def readable_file(string):
        if os.path.isfile(string) and os.access(string, os.R_OK):
            return string
        else:
            raise  TypeError("should be a readable file")

    def readable_directory(string):
        if os.path.isdir(string) and os.access(string, os.R_OK) and os.access(string, os.X_OK):
            return string
        else:
            raise  TypeError("should be an readable directory")


    def executable_file(string):
        if os.path.isfile(string) and os.access(string, os.X_OK):
            return string
        else:
            raise  TypeError("should be an executable file")


    # create the top-level parser
    parser = argparse.ArgumentParser(description='Run a program to test onto a series of .test files, or generates .test file for future tests.',
        formatter_class=argparse.RawDescriptionHelpFormatter
        )

    # create subparser for sucommand gentest and runtest
    subparsers = parser.add_subparsers(dest='subcommand',
        title='Sub command',
        #required=True, #Not available in python3.6
        #help='select whether to test by using .test files in a directory, or to generate .test files from .testarg files.'
        )
    subparsers.required = True


    # create the parser for the "test" subcommand
    parser_runtest_mode = subparsers.add_parser('runtest', aliases=['rt'],
        help='Runs all tests found in .test files in the oracle_test_dir, and compare output with oracle',
        description='''
Finds all the .test files in the provided oracle_test_dir.
Creates a 'result directory' named <oracle_test_dir>_result, which structure will be the same as the structure of the oracle_test_dir directory.
Then, on each test found in the .test files in the oracle_test_dir directory :
 * runs the program executable_to_test with the test's arguments
 * writes the outputs in a new .test file created inside the 'result directory'
 * and compare these outputs (stdout and stderr) with awaited outputs ("oracle") found in the original .test file.
Finally, a summary of all tests is generated.''',
        epilog='''See documentation (README) for a presentation of the .test files syntax.''',
        formatter_class=argparse.RawDescriptionHelpFormatter
        )
    parser_runtest_mode.add_argument('executable_to_test', help='Path to the excecutable program to test.',
        type=executable_file
        )
    parser_runtest_mode.add_argument('--valgrind', dest='run_valgrind', action='store_true', default=False,
        help='For each test, the program is also run into valgrind, and a .valgrindoutput file is created in case valgrind detected any error.'
        )
    parser_runtest_mode.add_argument('--strict-compare-stdout', '-s', dest='strict_cmp_outputs', action='store_true', default=False,
        help='Use strict comparison when comparing outuputs (stdout and stderr) of the test with outputs of the oracle.  Default is fuzzy compare : ignore whitespaces, and a couple of other weird characters.'
        )
    parser_runtest_mode.add_argument('--verbose', '-v', dest='verbose', action='count', default=0,
         help='Verbose outputs. Prints each test results.'
         )
    parser_runtest_mode.add_argument('oracle_dir', metavar='oracle_test_dir',
        help='Path to the oracle directory containing the .test files.',
        type=readable_directory
        )

    # create the parser for the "gentest" subcommand
    parser_gentest_mode = subparsers.add_parser('gentest', aliases=['gt'],
        help='Uses provided .testarg files to generate .test files for future tests.',
        description='''
For each .testarg file provided on the command line, a .test file will be created, in the same directory.
This .test file will be generated by running the executable_to_test on each line of the .testarg file.
It will contain the outputs (return code, stdout, stderr) of the executable_to_test.

These outputs can later be used as "oracle" for future tests of the executable_to_test.
To that aim, they should be carefully checked by hand.''',
        epilog='''
See documentation (README) for a presentation of the .testarg files syntax.''',
        formatter_class=argparse.RawDescriptionHelpFormatter

        )
    parser_gentest_mode.add_argument('executable_to_test', help='Path to the excecutable program to test.')
    parser_gentest_mode.add_argument('--verbose', '-v', dest='verbose', action='count', default=False,
        help='Verbose outputs. Prints each test results.'
        )
    parser_gentest_mode.add_argument('path_to_testarg_files', metavar='testarg_files',
        nargs='+',
        type=readable_file,
        help='Path to the one or several .testarg files'
        )

    args = parser.parse_args()

    # Hack : substitutes subcommand aliases by subcommand full name
    # Cf bug report https://bugs.python.org/issue36664
    # print (args.subcommand)
    if args.subcommand == 'rt':
        args.subcommand = 'runtest'
    if args.subcommand == 'gt':
        args.subcommand = 'gentest'

    return args



def runTest_getResultDirectoryPath( oracle_directory_path ):
    # return os.path.realpath(oracle_directory_path  + "_result")
    return oracle_directory_path + "_result"

def runTest_getResultFilePathForTestFile(oracle_directory_path, test_file_path ) :
    relative_path_to_test_file = test_file_path.replace(oracle_directory_path, "")
    return runTest_getResultDirectoryPath( oracle_directory_path ) + relative_path_to_test_file

def genTest_getResultFilePathForTestFile(test_args_file) :
    return os.path.splitext(test_args_file)[0] + ".test"


##################
# Useful formated printing functions
##################
def printDic(dictionary, message=None):
    if message :
        print(message + " => ", end = '')
    print(json.dumps(dictionary, indent=4))

def printAsciiCode(string, message=None):
    if message :
        print(message + " => ", end = '')
    print(' '.join(str(ord(c)) for c in string))


##################
# Management of SIGINT signals <CTRL+C> while runing a test
#     * stkip to next test on FIRST <CTRL+C>
#     * aborts the current python script if multiple hits of <CTRL+C> in less than 1 second
##################
_current_subprocess = None
_time_last_sigint = None
_sigint_duration_to_stop_this_script_sec = 1

def signal_handler_int(signum, frame):
    global _time_last_sigint
    cur_time = time.time()
    if _time_last_sigint != None :
        elapsed_sec = cur_time - _time_last_sigint
        if elapsed_sec < _sigint_duration_to_stop_this_script_sec :
            print(termcolor.colored("### <CTRL+c> was hit twice => abort this test script" , 'red'))
            exit(1)
    _time_last_sigint = cur_time

    if signum == signal.SIGINT and _current_subprocess != None :
        print(termcolor.colored("### <CTRL+c> was hit once => abort current test, start next test" , 'blue'))
        _current_subprocess.kill(signal.SIGTERM)


# Returns a list representing each test found in a test file
# Each item of the list is a two-level dictionnary containing the following entries
# source :
#   args           : MANDATORY. string containing the args to execute the test
#   return_code    : OPTIONAL - only if found. string containing the awaited ret code
#   output         : OPTIONAL - only if found. string containing the awaited stdout and stderr
#   test_file      : the path to the test file where the test data have been extracted, ie test_source_file_name
# result :
#   test_file      : the path to the test file which will gather the test results, ie test_output_file_name
def runTest_extractTestsDataFromTestFile(test_source_file_name, test_output_file_name):
    # fonction utilitaire pour extractOneTestDataFromOracleTestFile
    def extractAwaitedOutputFromOracleTestFile(file):
        awaited_output_L = []
        for line in file:
            if line.startswith("# TEST") or line.startswith("# Test") :
                print("Malformed test file when parsing test outputs when parsing : ")
                print("    " + line)
                print("The following line :")
                print("    # END Test stdout and stderr ")
                print("is missing. Aborts.")
                sys.exit(1)

            if line.startswith("# END Test stdout and stderr"):
                return "".join(awaited_output_L)
            awaited_output_L.append(line)
        return "".join(awaited_output_L)

    # fonction utilitaire pour runTest_extractTestsDataFromTestFile
    def extractOneTestDataFromOracleTestFile(file, test_D):
        # print("\n*** extractOneTestDataFromOracleTestFile")
        # reading one of the test from the file

        for line in file:
            if line.startswith("# Test args"):
                re_search_string = r"# Test args.*?:" + "(.*)?" + "\n"
                #print("re_search_string " + re_search_string)
                re_search = re.search(re_search_string, line)
                # print(re_search)
                if re_search:
                    test_D["args"] = re_search.group(1).strip()
                    # print("** args " + test_D["args"])
            elif line.startswith("# Test return code"):
                re_search_string = r"# Test return code.*?:" + "(.*)" + "\n"
                # print("re_search_string " + re_search_string)
                re_search = re.search(re_search_string, line)
                # print(re_search)
                if re_search:
                    test_D["return_code"] = re_search.group(1).strip()

            elif line.startswith("# Test stdout and stderr"):
                test_D["output"]=extractAwaitedOutputFromOracleTestFile(file)
                # print("** test_output " + test_D["output"])

            elif line.startswith("# TEST END"):
                break;
            elif line.strip() != "":
                print("Malformed test file when parsing test outputs when parsing : ")
                print("    " + line)
                print("Unrecognized. Aborts.")
                sys.exit(1)
        if not line.startswith("# TEST END"):
            print("Malformed test file. The line starting with : ")
            print(termcolor.colored("# TEST END", 'blue'))
            print("was not found. Aborts.")
            sys.exit(1)

        if not "args" in test_D:
            print("Malformed test file. The line starting with : ")
            print(termcolor.colored("# Test args              :", 'blue'))
            print("was not found. Aborts.")
            sys.exit(1)

        # print("** test found :" + str(test_D))
        return test_D


    test_data_L = []
    with open(test_source_file_name, "r", encoding='utf-8', errors='replace', newline='') as file:
        for line in file:
            if line.startswith("# TEST START"):
                test_D = {
                    "test_file" : test_source_file_name
                }
                extractOneTestDataFromOracleTestFile(file, test_D)
                if "args" in test_D:
                    test_data_L.append({
                        "source" : test_D,
                        "result" : {
                            "test_file" : test_output_file_name
                         }
                    })
    return test_data_L



# Returns a list representing each test found in a test file
# Each item of the list is a two-level dictionnary containing the following entries
# source :
#   args           : MANDATORY. string containing the args to execute the test
# result :
#   test_file      : the path to the test file which will gather the test results, ie test_output_file_name
def genTest_extractAllTestArgsFromTestArgFile(testargfile_name, test_output_file_name):
    test_data_L = []
    with open(testargfile_name, "r", encoding='utf-8', errors='replace', newline='') as file:
        for line in file:
            if line == "\n" or line.startswith("#"):
                continue
            # suppress final '\n' char on the args line
            if line.endswith("\n"):
                line = line[:-1]

            test_D =  { }
            test_D["args"] = line
            test_D["test_file"] = testargfile_name
            test_data_L.append({
                "source" : test_D,
                "result" : {
                    "test_file" : test_output_file_name
                 }
            })

    return test_data_L



# Drop ANSI colors etc from a bash string
# def escape_bash_ansi(text):
#     ansi_escape = re.compile(r'(?:\x1B[@-_]|[\x80-\x9F])[0-?]*[ -/]*[@-~]')
#     return ansi_escape.sub('', text)

###############
# Runs the executable_to_test with arguments specified in args
# and returns a dictionnary containing the following keys
#     "executed_command"
#     "return_code"
#     "output"
###############
def executeCmdAndGetResults(executable_to_test, args) :
    global _current_subprocess
    return_dic = {}
    # print("executeCmdAndGetResults args =  " + args)

    # parse la chaine en appliquant la syntaxe Shell
    args_L = shlex.split(args)
    args_escaped_L = []
    for i in range(0, len(args_L)):
        args_escaped_L.append("'" + args_L[i] + "'")

    return_dic["executed_command"] = executable_to_test + "  " + " ".join(args_escaped_L);
    # print(" executeCmdAndGetResults return_dic["executed_command"] = "+ return_dic["executed_command"])

    # execution de la commande
    _current_subprocess = pexpect.spawn(executable_to_test, args_L)
    ## Ce qui suit permet de s'assurer qu'on récupère bien stdout et stderr en totalité
    _current_subprocess.expect(pexpect.EOF)
    _current_subprocess.wait()

    # traitement des résultats
    return_dic["return_code"] = "EXIT_SUCCESS"
    retcode = str(_current_subprocess.exitstatus)
    if _current_subprocess.exitstatus is None:
        return_dic["return_code"] = "SIGNAL " + str(_current_subprocess.signalstatus) + " - " + signal.Signals(_current_subprocess.signalstatus).name
    elif retcode != str(0):
        return_dic["return_code"] = "EXIT_FAILURE"


    # we use decode with 'replace' to ensure keeping all bytes
    stdout  = _current_subprocess.before.decode(encoding="utf-8", errors='replace')
    _current_subprocess = None

    # NORMALISE OUTPUTS : get rid of weirdest chars
    # 0x08   backspace      because would be removed by most editors
    #                       in case the .test file is edited...
    # 0x13   \r'            because would be removed in case the file is edited...
    stdout = stdout.replace('\r', '');          # remove '\r'
    stdout = stdout.replace(chr(0x08), '');     # remove backspace char

    return_dic["output"] = stdout

    return return_dic


###############
# Runs the executable_to_test with arguments specified in args
# within valgrind
# and returns a dictionnary containing the following keys
#     "valgrind_error" true or false
#     "valgrind_error_or_leak" true or false
#     "valgrind_output"
###############
def executeCmdWithinValgrind(executable_to_test, args) :
    global _current_subprocess
    return_dic = {}
    # print("executeCmdWithinValgrind args =  " + args)

    # parse la chaine en appliquant la syntaxe Shell
    args_L = shlex.split(args)

    args_L.insert(0, executable_to_test)
    args_L.insert(0, "--error-exitcode=127")

    # execution de la commande
    _current_subprocess = pexpect.spawn("valgrind", args_L)
    ## Ce qui suit permet de s'assurer qu'on récupère bien stdout et stderr en totalité
    _current_subprocess.expect(pexpect.EOF)
    _current_subprocess.wait()
    # traitement des résultats
    return_dic["valgrind_error"] = False
    if str(_current_subprocess.exitstatus) == "127":
        return_dic["valgrind_error"] = True

    # seconde execution de valgrind avec détection des memory-leaks
    args_L.insert(0, "--errors-for-leak-kinds=all")
    args_L.insert(0, "--leak-check=full")
    # execution de la commande
    # print("arg_L" + str(args_L))
    _current_subprocess = pexpect.spawn("valgrind", args_L)
    ## Ce qui suit permet de s'assurer qu'on récupère bien stdout et stderr en totalité
    _current_subprocess.expect(pexpect.EOF)
    _current_subprocess.wait()
    # traitement des résultats
    # print("EXIT " + str(_current_subprocess.exitstatus))
    return_dic["valgrind_error_or_leak"] = False
    if str(_current_subprocess.exitstatus) == "127":
        return_dic["valgrind_error_or_leak"] = True
        # print("AIE")

    # we use decode with 'replace' to ensure keeping all bytes
    stdout  = _current_subprocess.before.decode(encoding="utf-8", errors='replace')
    _current_subprocess = None

    # NORMALISE OUTPUTS : get rid of weirdest chars
    # 0x08   backspace      because would be removed by most editors
    #                       in case the .test file is edited...
    # 0x13   \r'            because would be removed in case the file is edited...
    stdout = stdout.replace('\r', '');          # remove '\r'
    stdout = stdout.replace(chr(0x08), '');     # remove backspace char

    return_dic["valgrind_output"] = stdout

    return return_dic

###############
# Runs the executable_to_test with arguments specified in the test_data_D dictionnary
# and stores results in a test_data_D["result"] dictionnary,
# with in particular the following keys :
#   args                : string containing the args employed to run executable_to_test
#   command_line        : string containing the executed command
#   return_code         : string containing the return code of executable_to_test
#   output              : string containing the outputs (stdout and stderr) of executable_to_test
###############
def executeTest(executable_to_test, test_data_D):
    global _current_subprocess

    # prepare the dictionnary of test results
    test_data_D["result"]["args"] = test_data_D["source"]["args"]

    print(test_data_D["result"]["args"])
    print(termcolor.colored("### Running executable with arguments : ", 'blue') + " ".join(shlex.split(test_data_D["result"]["args"])) )

    ret_D = executeCmdAndGetResults(executable_to_test, test_data_D["result"]["args"])
    test_data_D["result"]["command_line"]    = ret_D["executed_command"]
    test_data_D["result"]["return_code"]     = ret_D["return_code"]
    test_data_D["result"]["output"]          = ret_D["output"]

    print("    Executed command : ")
    print("        " + test_data_D["result"]["command_line"]  )
    
    #print("    Command finished." )

def re_executeTestWithinValgrind(executable_to_test, test_data_D):
    print(termcolor.colored("### ... Running again executable within valgrind... : ", 'blue'))
    ret_D = executeCmdWithinValgrind(script_args.executable_to_test, test_data_D["result"]["args"])
    test_data_D["result"]["valgrind_error"] = ret_D["valgrind_error"]
    test_data_D["result"]["valgrind_error_or_leak"] = ret_D["valgrind_error_or_leak"]
    test_data_D["result"]["valgrind_output"] = ret_D["valgrind_output"]

###############
# Append the result of a test, ie test_data_D["result"],
# in the restult file test_output_file_name
# with .test files format
###############
def appendToResultFile(test_data_D):
    #print(test_data_D)
    #print("***APPEND *** " + test_data_D["result"]["test_file"] )
    f = open(test_data_D["result"]["test_file"], "a", encoding='utf-8', errors='replace', newline='')
    f.write("\n")
    f.write("\n")

    f.write("# TEST START ###################################################################" + "\n")
    f.write("# Test args              : " + test_data_D["result"]["args"] + "\n")
    # f.write("# Test command line      : \n")
    # f.write("     ./" + " ".join(local_command_L) + "\n")
    f.write("# Test return code       : " + test_data_D["result"]["return_code"] + "\n")

    if "output" in test_data_D["result"]  :
        f.write("# Test stdout and stderr :" + "\n")
        f.write( test_data_D["result"]["output"]);
        if not test_data_D["result"]["output"].endswith('\n') :
            f.write("\n");
        f.write("# END Test stdout and stderr" + "\n")

    f.write("# TEST END   ###################################################################" + "\n")
    f.close()

###############
# Increment the dictionnary tests_results_summary_D
# by analysing the results of the test found in the dictonnary test_data_D["result"]
# eg : analyse segfault,
#      analyse return codes and stdout by comparing with test_data_D["source"],
#      etc.
###############
def analyseTestResults(test_data_D, tests_results_summary_D, print_results_to_stdout=True):
    tests_results_summary_D["all"].append(test_data_D)

    test_is_ok = True

    if test_data_D["result"]["return_code"].startswith("SIGNAL") :
        if print_results_to_stdout : print(termcolor.colored("    Test return code : " + test_data_D["result"]["return_code"], 'red'))
        tests_results_summary_D["invalid_segfault"].append(test_data_D)
        test_is_ok = False
    elif not "return_code" in test_data_D["source"]:
        if print_results_to_stdout : print(termcolor.colored("    Test return code : cannot check, data not available in test source", 'cyan'))
    else:
        if test_data_D["source"]["return_code"].strip(" ") == test_data_D["result"]["return_code"].strip(" ") :
            if print_results_to_stdout : print("    Test return code : correct")
        else :
            if print_results_to_stdout : print(termcolor.colored("    Test return code : ERROR.", 'red') + " got '" + test_data_D["result"]["return_code"] + "' expected '" + test_data_D["source"]["return_code"] + "'")
            tests_results_summary_D["invalid_retcode"].append(test_data_D)
            test_is_ok = False

    if not "output" in test_data_D["source"] :
        if print_results_to_stdout : print(termcolor.colored("    Test output      : cannot check, data not available in test source", 'cyan'))
    else :
        # by default, compare outputs ignoring all whitespace
        test_source_stdout = re.sub(r"\s+", '-', test_data_D["source"]["output"])
        test_result_stdout = re.sub(r"\s+", '-', test_data_D["result"]["output"])
        if script_args.strict_cmp_outputs == True :
            # if option --strict-compare-stdout : strict comparaison
            test_source_stdout = test_data_D["source"]["output"]
            test_result_stdout = test_data_D["result"]["output"]

        if test_source_stdout == test_result_stdout :
            if print_results_to_stdout : print("    Test output      : correct")
        else :
            if print_results_to_stdout : print(termcolor.colored("    Test output      : ERROR.", 'red'))
            tests_results_summary_D["invalid_outputs"].append(test_data_D)
            test_is_ok = False

            # print("**** oracle stdout")
            # print('#')
            # print(test_data_D["source"]["output"])
            # print('#')
            #printAsciiCode(test_data_D["source"]["output"], "test_data_D[source][output]")
            #printAsciiCode(test_source_stdout, "test_source_stdout")
            #
            # print("**** test stdout")
            # print('#')
            # print(test_data_D["result"]["output"])
            # print('#')
            #printAsciiCode(test_data_D["result"]["output"], "test_data_D[result][output] ")
            #printAsciiCode(test_result_stdout, "test_result_stdout")
    if "valgrind_error" in test_data_D["result"] and test_data_D["result"]["valgrind_error"] == True:
        print(termcolor.colored("    Valgrind detected a memory error with this test.", 'red'))
        tests_results_summary_D["invalid_valgrind_error"].append(test_data_D)
        
    if "valgrind_error_or_leak" in test_data_D["result"] and test_data_D["result"]["valgrind_error_or_leak"] == True:
        print(termcolor.colored("    Valgrind detected either a memory error or memory leak with this test.", 'red')) 
        tests_results_summary_D["invalid_valgrind_error_or_leak"].append(test_data_D)

    if script_args.verbose > 0 :
        print(termcolor.colored("--> Verbose test results :", 'green'))
        printDic(test_data_D)

    if test_is_ok:
        tests_results_summary_D["ok"].append(test_data_D)

###############
# Final print in runtest mode
###############
def runTest_printSummary(tests_results_summary_D) :
    print()
    print(termcolor.colored("################################################################", 'blue'))
    print(termcolor.colored("TEST RESULTS SUMMARY", 'blue'))
    print(termcolor.colored("################################################################", 'blue'))

    if len(tests_results_summary_D["ok"])>0:
        print(termcolor.colored("Tests successful :", 'green'))
        for test in tests_results_summary_D["ok"]:
            print("  " + test["result"]["command_line"])

    if len(tests_results_summary_D["invalid_segfault"])>0:
        print(termcolor.colored("Tests with segfaults, signals, or user-terminated (eg infinite loops) :", 'red'))
        for test in tests_results_summary_D["invalid_segfault"]:
            print("  " + test["result"]["command_line"])
            print("      (found in test file : " + test["source"]["test_file"] + ")")

    if len(tests_results_summary_D["invalid_retcode"])>0:
        print(termcolor.colored("Tests with incorrect return code :", 'red'))
        for test in tests_results_summary_D["invalid_retcode"]:
            print("  " + test["result"]["command_line"])
            print("      (found in test file : " + test["source"]["test_file"] + ")")
    if len(tests_results_summary_D["invalid_outputs"])>0 :
        print(termcolor.colored("Tests with incorrect output :", 'red'))
        for test in tests_results_summary_D["invalid_outputs"]:
            print("  " + test["result"]["command_line"])
            print("      (found in test file : " + test["source"]["test_file"] + ")")

    if len(tests_results_summary_D["invalid_valgrind_error"])>0 :
        print(termcolor.colored("Tests with memory access errors detected by valgrind :", 'red'))
        for test in tests_results_summary_D["invalid_valgrind_error"]:
            print("  " + test["result"]["command_line"])
            print("      (found in test file : " + test["source"]["test_file"] + ")")


    if len(tests_results_summary_D["invalid_valgrind_error_or_leak"])>0 :
        print(termcolor.colored("Tests with memory access errors OR memory leaks detected by valgrind :", 'red'))
        for test in tests_results_summary_D["invalid_valgrind_error_or_leak"]:
            print("  " + test["result"]["command_line"])
            print("      (found in test file : " + test["source"]["test_file"] + ")")


    print(termcolor.colored("################################################################", 'blue'))
    # print(termcolor.colored("*** Number of tests run         : " + str(len(tests_results_summary_D["all"])), 'blue'))
    # print(termcolor.colored("*** Among which : ", 'blue'))
    print(termcolor.colored("*** Number of tests successful  : " + str(len(tests_results_summary_D["ok"])) + " out of " + str(len(tests_results_summary_D["all"])), 'green'))
    print(termcolor.colored("*** Number of tests with errors : " + str(len(tests_results_summary_D["all"]) - len(tests_results_summary_D["ok"])) + " out of " + str(len(tests_results_summary_D["all"])), 'red'))
    print(termcolor.colored("***  Among which : ", 'blue'))
    color = 'red' if len(tests_results_summary_D["invalid_segfault"])>0 else 'blue'
    print(termcolor.colored("***   Number of tests with segfault, signal, or user-terminated : " + str(len(tests_results_summary_D["invalid_segfault"])), color))
    color = 'red' if len(tests_results_summary_D["invalid_retcode"])>0 else 'blue'
    print(termcolor.colored("***   Number of tests with incorrect return code                : " + str(len(tests_results_summary_D["invalid_retcode"])), color))
    color = 'red' if len(tests_results_summary_D["invalid_outputs"])>0 else 'blue'
    print(termcolor.colored("***   Number of tests with incorrect return output              : " + str(len(tests_results_summary_D["invalid_outputs"])), color))
    print(termcolor.colored("################################################################", 'blue'))

    if len(tests_results_summary_D["invalid_valgrind_error_or_leak"])>0 :
        color = 'red' if len(tests_results_summary_D["invalid_valgrind_error"])>0 else 'blue'
        print(termcolor.colored("***   Number of tests with with memory access error reported by valgrind             : " + str(len(tests_results_summary_D["invalid_valgrind_error"])), color))

        color = 'red' if len(tests_results_summary_D["invalid_valgrind_error_or_leak"])>0 else 'blue'
        print(termcolor.colored("***   Number of tests with with memory access error OR memory leaks reported by valgrind             : " + str(len(tests_results_summary_D["invalid_valgrind_error_or_leak"])), color))


    print("You may want to compare test oracles and test results by running : ")
    print("  meld " + script_args.oracle_dir + " " + runTest_getResultDirectoryPath(script_args.oracle_dir))


###############
# Final print in gentest mode
###############
def genTest_printSummary(tests_results_summary_D) :
    print()
    generated_test_files_S = set()
    for test_data_D in tests_results_summary_D["all"]:
        generated_test_files_S.add(test_data_D["result"]["test_file"])

    print(termcolor.colored("################################", 'blue'))
    if len(tests_results_summary_D["invalid_segfault"])>0:
        print(termcolor.colored("Tests with segfaults, signals, or user-terminated (eg infinite loops) :", 'red'))
        for test in tests_results_summary_D["invalid_segfault"]:
            print("  " + test["result"]["command_line"])
            print("      (found in testarg file : " + test["source"]["test_file"] + ")")
    print(termcolor.colored("################################", 'blue'))
    print(termcolor.colored("The following .test files have been generated : ", 'blue'))
    print(termcolor.colored("   " + "\n   ".join(generated_test_files_S), 'blue'))

    if len(tests_results_summary_D["invalid_segfault"]) > 0 :
        color = 'red'
        print(termcolor.colored("*** Care : " + str(len(tests_results_summary_D["invalid_segfault"])) + " execution(s) of the program ended with segfault, signal, etc. See list above. " , color))
    print(termcolor.colored("################################", 'blue'))
    print(termcolor.colored("Check carefully the results in .test files.", 'blue'))
    print(termcolor.colored("Once validated, these .test files can later be used as 'oracle' for future tests.", 'blue'))
    print(termcolor.colored("################################", 'blue'))



###############
#### MAIN
###############

script_args = parseScriptArgs()
print(script_args)

# Normalise end of oracle directory path : remove trailing /
if script_args.subcommand == 'runtest':
    script_args.oracle_dir = script_args.oracle_dir.rstrip('/')

# The list of test data (list of dictonnaries)
test_data_L= []
# Structure of two-level dictionnaries in this list  :
#
#  source : a dictionnary built from the test data found in the source file
#   test_file      : the path to the file from where the test data have been extracted, ie a .test or .testarg file
#   args           : MANDATORY. string containing the args to execute the test
#   return_code    : OPTIONAL - only if found. string containing the awaited ret code
#   output         : OPTIONAL - only if found. string containing the awaited stdout and stderr
# result : a dictionnary built by running the executable_to_test on the arguments  args in "source"
#  the "result" dictionnay will contain the following keys once the executable has been run :
#   test_file      : the path to the test file which will gather the test results, ie test_output_file_name
#   args                : string containing the args employed to run executable_to_test
#   command_line        : string containing the executed command
#   return_code         : string containing the return code of executable_to_test
#   output              : string containing the outputs (stdout and stderr) of executable_to_test
#  et optionnellement (option --valgrind):
#      valgrind_error          : True si valgrind rapporte des erreurs d'accès mémoire
#      valgrind_error_or_leak  : True si valgrind rapporte des erreurs d'accès mémoire ou fuite mémoire
#      valgrind_output         : La sortie de valgrind


# Extraction of the tests' data to run
if script_args.subcommand == 'runtest':
    for testfile in glob.iglob( script_args.oracle_dir + "/*.test", recursive=True):
        print("##############################################")
        print("### Analysing test file :" + testfile)
        print("##############################################")

        # get the list of dictionnaries representing all tests found in testfile
        test_output_file_name = runTest_getResultFilePathForTestFile(script_args.oracle_dir, testfile )

        test_data_L.extend(runTest_extractTestsDataFromTestFile(testfile, test_output_file_name))

elif script_args.subcommand == 'gentest':
    for testargfile in script_args.path_to_testarg_files :
        print("##############################################")
        print("### Analysing test arg file :" + testargfile)
        print("##############################################")

        # get the list of dictionnaries representing the test results to generate
        test_output_file_name = genTest_getResultFilePathForTestFile(testargfile)
        test_data_L.extend(genTest_extractAllTestArgsFromTestArgFile(testargfile, test_output_file_name))


if script_args.verbose > 0 :
    print(termcolor.colored("### List of tests about to be run :", 'green'))
    for test_data_D in test_data_L :
        printDic(test_data_D)
    x = input("Press <Enter> to continue.")


# Confirm suppression of existing results
if script_args.subcommand == 'runtest':
    # print("script_args.executable_to_test : " + script_args.executable_to_test)
    # print("script_args.oracle_dir : " + script_args.oracle_dir)
    result_directory = runTest_getResultDirectoryPath( script_args.oracle_dir )
    if os.path.isdir(result_directory):
        print("The result directory ")
        print("  " + result_directory)
        print("already exists.")
        print(termcolor.colored("Remove this directory and continue ? (<Enter> to continuer, <n+Enter> to stop)", 'blue'))
        x = input("   ")
        if x == 'n':
            print("Aborted.")
            sys.exit(0)
        shutil.rmtree(result_directory)

    # re-create an empty directory that will contain the results of the tests
    # os.mkdir(runTest_getResultDirectoryPath( script_args.oracle_dir ))
    # BETTER (easier meld after running the tests) : copy recursively source directory into <source>_result directory
    # then remove .test in <source>_result directory
    shutil.copytree(script_args.oracle_dir, result_directory)
    for test_file in glob.glob(result_directory + "/*.test"):
        os.remove(test_file)

elif script_args.subcommand == 'gentest':
    # check if .test files will be deleted and recreated
    existing_test_output_file_name_L = []
    for testargfile in script_args.path_to_testarg_files :
        test_output_file_name = genTest_getResultFilePathForTestFile(testargfile)
        if os.path.isfile(test_output_file_name):
            existing_test_output_file_name_L.append(test_output_file_name)

    # confirm deletion of existing .test files
    if len(existing_test_output_file_name_L)>0 :
        print("The following .test files :")
        print("   " + "  ".join(existing_test_output_file_name_L))
        print("will be deleted and regenerated.")
        print(termcolor.colored("Confirm suppression ? (<Enter> to continuer, <n+Enter> to stop)", 'blue'))
        if x == 'n':
            print("Aborted.")
            sys.exit(0)

    # print(existing_test_output_file_name_L)
    for test_output_file_name in existing_test_output_file_name_L:
        os.remove(test_output_file_name)


# Run tests (or gen tests)

print("### Hit <Ctrl+c> twice in less than " + str(_sigint_duration_to_stop_this_script_sec) + " seconds to abort this script.")
print("### Hit <Ctrl+c> once to abort the current test (e.g. if infinite loop).")
print()
time.sleep(1)


tests_results_summary_D = {
    "all" : [],
    "ok" : [],
    "invalid_segfault" : [],
    "invalid_retcode" : [],
    "invalid_outputs" : [],
    "invalid_valgrind_error" : [],
    "invalid_valgrind_error_or_leak" : []
}


# Setup sigint signal handler
signal.signal(signal.SIGINT, signal_handler_int)


if script_args.subcommand == 'runtest':
    for test_data_D in test_data_L:
        #print("### Hit <Ctrl+c> once to abort the current test.")
        #print("### Hit <Ctrl+c> twice in less than " + str(_sigint_duration_to_stop_this_script_sec) + " seconds to abort this script.")
        time.sleep(_sleep_between_tests)

        executeTest(script_args.executable_to_test, test_data_D)
        #printDic(test_data_D)

        if script_args.run_valgrind == True:
            re_executeTestWithinValgrind(script_args.executable_to_test, test_data_D)

        appendToResultFile(test_data_D)
        analyseTestResults(test_data_D, tests_results_summary_D)

    runTest_printSummary(tests_results_summary_D)

elif script_args.subcommand == 'gentest':
    for test_data_D in test_data_L:
        #print("### Hit <Ctrl+c> once to skip current call to executable.")
        #print("### Hit <Ctrl+c> twice in less than " + str(_sigint_duration_to_stop_this_script_sec) + " seconds to abort this script.")
        time.sleep(_sleep_between_tests)
        executeTest(script_args.executable_to_test, test_data_D)
        #printDic(test_data_D)

        appendToResultFile(test_data_D)
        analyseTestResults(test_data_D, tests_results_summary_D, False)

    genTest_printSummary(tests_results_summary_D)
