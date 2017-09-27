from os.path import *
from glob import glob
from bs4 import BeautifulSoup
from functools import reduce
import re

IGNORE_DIR = ["Alloy", "Arsenic", "Zinc", "Wasabi"]

def isIgnore(value, ignoreList=[]):
    for i in ignoreList:
        if value in i:
            return True
    return False

def getAllInterfaceXML():
    AllInterfaceXML = []
    DIR = []
    root_path=abspath(".")
    OEMSrc=glob(root_path+"/*")

    for each_item in OEMSrc:

        #Remove ingnore directories
        if( isIgnore(each_item, IGNORE_DIR) != False ):
            continue

        app_name = each_item.split("/")[-1]
        interface=glob(root_path+"/"+app_name+"/"+app_name+".System.API/data/introspection-xml/*")
        if len(interface) is not 0:
            AllInterfaceXML.extend(interface)

    return AllInterfaceXML

def simple_print_all_methods(xml_filename):
    soup = BeautifulSoup(open(xml_filename), "lxml")
    interface=soup.find("interface")

    #DBUS object
    dbus_busname_temp=interface.attrs["name"].split(".")[0:-1]
    dbus_busname=reduce(lambda x, y: x+"."+y, dbus_busname_temp)
    dbus_object="/"+interface.attrs["name"].replace(".","/")

    all_methods=list()
    for method in interface.findAll("method"):
        ret=dict()
        #print("\t"+method["name"])
        #print("\t\t"+method.find("yv:docstring").get_text())
        arg_list=list()
        return_str="void:return_value"
        
        doc_string=""
        doc_tag=method.find("yv:docstring")

        if doc_tag is not None:
            doc_string=doc_tag.get_text()

        for arg in method.findAll("arg"):
            if "direction" in arg.attrs:
                if arg["direction"] in "out":
                    #print("\t\t"+arg["type"]+":return_value")
                    return_str=arg["type"]+":return_value"
                    continue
            #print("\t\t\t"+arg["type"]+":"+arg["name"])
            arg_list.append(arg["type"]+":"+arg["name"])
            
        ret["method"]=method["name"]
        ret["doc"]=doc_string
        ret["args"]=arg_list
        ret["return"]=return_str     
        all_methods.append(ret)

    #### print ####
    print("busname : " + dbus_busname)
    print("object : " + dbus_object)
    print("\n\n")

    for each_method in all_methods:
        #args
        args=""
        for arg in each_method["args"]:
            args = args+"\t"+arg+"\n"

        #abs_method
        abs_method=str(dbus_object.replace("/",".")+"."+each_method["method"])[1:]
        result="dbus-send --session --print-reply --type=method_call --dest="+dbus_busname+" "+dbus_object+" "+abs_method

        ################### print ###################
        if len(each_method["doc"]) is not 0:
            print("Description : "+each_method["doc"])
        print("["+each_method["return"]+"]")
        print(result)
        print(args)
        print("\n\n")
        #############################################


def print_all_methods(xml_filename):
    soup = BeautifulSoup(open(xml_filename), "lxml")

    interface=soup.find("interface")

    #DBUS object
    dbus_busname=interface.attrs["name"]
    dbus_object="/"+dbus_busname.replace(".","/")

    #DBUS methods
    dbus_methods=[]
    
    for each_item in interface.findAll("method"):
        dbus_methods.append(dbus_object+"/"+each_item.attrs["name"])



    ###### print #########
    print("busname : " + dbus_busname)
    print("object : " + dbus_object)

    for method in dbus_methods:
        method_name = method.split("/")[-1]
        docstring=""

        #extract docstring
        for each_item in interface.findAll("method", {"name":method_name}):
            doc=each_item.find("yv:docstring")
            if doc is not None:
                docstring = doc.get_text()

        #make dbus-send command
        result="dbus-send --session --print-reply --type=method_call --dest=="+dbus_busname+" "+dbus_object+" "+method

        if len(docstring) :
            print(docstring)
        print(result)

        #make agruments, return value and doc string

        #DBUS DOC string
        return_value = "void"

        for each_item in interface.findAll("method", {"name":method_name}):
            for arg in each_item.findAll("arg"):
                # get return value
                if( "direction" in arg.attrs ):
                    if( arg["direction"] in "out" ):
                        return_value = arg["type"]
                        continue

                # get arguments
                print("\t"+arg["type"]+":"+arg["name"])

            #print("\treturn : "+return_value+"\n")
            print("\n\n")

    
if __name__ == "__main__":
    #print_all_methods("Indium/Indium.System.API/data/introspection-xml/interface-CWMPManager.xml")
    ListXML=getAllInterfaceXML()
    ListXMLInterface=[]

    p = re.compile("interface*")

    for i in ListXML:
        if p.search(i) is not None:
            ListXMLInterface.append(i)

    for each_xml in ListXMLInterface:
        simple_print_all_methods(each_xml)
        #print_all_methods(each_xml)
        print("-"*100)

