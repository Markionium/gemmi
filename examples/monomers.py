#!/usr/bin/env python
from __future__ import print_function
import os
import sys
import argparse
from collections import Counter
from gemmi import cif
import util


def to_formula(cnt):
    '{C:12, O:8, N:1} -> "C12 N O8"'
    return ' '.join(k + (str(v) if v != 1 else '')
                    for k, v in sorted(cnt.items()))


def get_monomer_cifs(mon_path):
    'yield all files mon_path/?/*.cif in alphabetic order'
    for root, name in util.sorted_cif_search(mon_path):
        if '_' not in name:  # heuristic to skip weird things in $CLIBD_MON
            yield os.path.join(root, name)


def check_formulas(ccd):
    '''\
    Print the chemical components in CCD (components.cif) that have
    formula not consistent with the list of atoms.
    '''
    for b in ccd:
        atoms = Counter(a.as_str(0).upper() for a in
                        b.find('_chem_comp_atom.type_symbol'))
        formula = cif.as_string(b.find_value('_chem_comp.formula')).upper()
        fdict = util.formula_to_dict(formula)
        if fdict != atoms:
            print('[%s]' % b.name, formula, '<>', to_formula(atoms))


def compare_monlib_with_ccd(mon_path, ccd):
    'compare monomers from monomer library and CCD that have the same names'
    PRINT_MISSING_ENTRIES = False
    cnt = 0
    for path in get_monomer_cifs(mon_path):
        mon = cif.read_any(path)
        for mb in mon:
            if mb.name in ('', 'comp_list'):
                continue
            assert mb.name.startswith('comp_')
            name = mb.name[5:]
            cb = ccd.find_block(name)
            if cb:
                compare_chem_comp(mb, cb)
                cnt += 1
            elif PRINT_MISSING_ENTRIES:
                print('Not in CCD:', name)
    print('Compared', cnt, 'monomers.')


def get_heavy_atom_names(block):
    cca = block.find('_chem_comp_atom.', ['atom_id', 'type_symbol'])
    d = {a.as_str(0).upper(): a.as_str(1).upper() for a in cca if a[1] != 'H'}
    assert len(d) == sum(a[1] != 'H' for a in cca), (d, cca)
    return d


def bond_info(id1, id2, order, aromatic):
    if id1 > id2:
        id1, id2 = id2, id1
    order = order[:4].lower()
    assert order in ['sing', 'doub', 'trip', 'arom', 'delo', '1.5'], order
    aromatic = aromatic.upper()
    assert aromatic in ('Y', 'N'), aromatic
    if order in ('sing', 'doub') and aromatic == 'Y':
        order = 'arom'
    return ((id1, id2), (order, aromatic))


def bond_dict(block, ccb_names, atom_names):
    table = block.find('_chem_comp_bond.', ccb_names)
    return dict(bond_info(r.as_str(0), r.as_str(1), r.as_str(2), r.as_str(3))
                for r in table
                if r.as_str(0) in atom_names and r.as_str(1) in atom_names)


def compare_chem_comp(mb, cb):
    if verbose:
        print('Comparing', cb.name)
    mon_names = get_heavy_atom_names(mb)
    ccd_names = get_heavy_atom_names(cb)

    # compare atom names (and counts)
    if mon_names != ccd_names:
        print('atom names differ in', cb.name)
        mon_atoms = Counter(mon_names.values())
        ccd_atoms = Counter(ccd_names.values())
        if mon_atoms != ccd_atoms:
            # can be relaxed by: and mon_atoms + Counter('O') != ccd_atoms
            print(cb.name, to_formula(mon_atoms), '<>', to_formula(ccd_atoms))

    # compare bonds
    mbonds = bond_dict(mb, ['atom_id_1', 'atom_id_2', 'type', 'aromatic'],
                       mon_names)
    cbonds = bond_dict(cb, ['atom_id_1', 'atom_id_2', 'value_order',
                            'pdbx_aromatic_flag'],
                       ccd_names)
    if mbonds != cbonds:
        print('Bonds differ in', cb.name)
        if set(mbonds) != set(cbonds):
            print('  extra bonds in mon.lib.:',
                  ' '.join('%s-%s' % p for p in set(mbonds) - set(cbonds)))
            print('  extra bonds in CCD:',
                  ' '.join('%s-%s' % p for p in set(cbonds) - set(mbonds)))
        for apair in sorted(set(mbonds) & set(cbonds)):
            if mbonds[apair] != cbonds[apair]:
                print('  %s-%s: %s/%s <> CCD %s/%s' %
                      (apair + mbonds[apair] + cbonds[apair]))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('ccd_path', metavar='/path/to/components.cif[.gz]')
    parser.add_argument('-m', metavar='DIR',
                        help='monomer library path (default: $CLIBD_MON)')
    parser.add_argument('-f', action='store_true', help='check CCD formulas')
    parser.add_argument('-v', action='store_true', help='verbose')
    args = parser.parse_args()
    global verbose
    verbose = args.v
    mon_path = args.m or os.getenv('CLIBD_MON')
    if not mon_path and not args.f:
        sys.exit('Unknown monomer library path: use -m or set $CLIBD_MON.')
    ccd = cif.read_any(args.ccd_path)
    if args.f:
        check_formulas(ccd)
    if mon_path:
        compare_monlib_with_ccd(mon_path, ccd)


main()
