import sigrokdecode as srd


class Decoder(srd.Decoder):
    api_version = 3
    id = 'wait_match'
    name = 'wait() matcher test'
    longname = 'wait() matcher integration test'
    desc = 'Exercises wait() matching semantics.'
    license = 'gplv2+'
    inputs = ['logic']
    outputs = []
    tags = ['Util']
    channels = (
        {'id': 'a', 'name': 'A', 'desc': 'First required channel'},
        {'id': 'b', 'name': 'B', 'desc': 'Second required channel'},
    )
    optional_channels = (
        {'id': 'unused', 'name': 'Unused', 'desc': 'Unmapped optional channel'},
    )
    annotations = (('match', 'Match result'),)
    annotation_rows = (('matches', 'Matches', (0,)),)

    def __init__(self):
        self.reset()

    def reset(self):
        self.out_ann = None

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def report(self):
        matched = ''.join('1' if value else '0' for value in self.matched)
        self.put(self.samplenum, self.samplenum, self.out_ann,
                 [0, [matched]])

    def decode(self):
        self.wait([{0: 'r'}, {'skip': 2}])
        self.report()

        self.wait({1: 'r'})
        self.report()

        self.wait({0: 'h', 1: 'h'})
        self.report()

        self.wait([{2: 'h'}, {'skip': 1}])
        self.report()

        while True:
            self.wait()
