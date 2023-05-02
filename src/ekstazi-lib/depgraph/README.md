
# Dependency Graph Generator

## Building
Simply use the provided make file to build the program.
```
make
```

## Running
To run the program, call the executable and provide the dependency files.
```
./depgraph-generator -n animal.cc.ekstazi.txt hello.cc.ekstazi.txt -o animal.cc.ekstazi.txt.old hello.cc.ekstazi.txt.old
```

The program should produce an output of the dependency graph:
```
_ZN5HelloD2Ev
  ->_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
_ZN5Hello5printEv
  ->_ZStlsIcSt11char_traitsIcESaIcEERSt13basic_ostreamIT_T0_ES7_RKNSt7__cxx1112basic_stringIS4_S5_T1_EE
  ->_ZNSolsEPFRSoS_E
_ZN5HelloC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
  ->_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1Ev
  ->_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSERKS4_
  ->_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
  ->__clang_call_terminate
main
  ->_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc
  ->_ZNSolsEPFRSoS_E
  ->_ZNSaIcEC1Ev

...

```
